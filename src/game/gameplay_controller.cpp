#include "mw3gf/game/gameplay_controller.hpp"
#include "mw3gf/game/native_layout.hpp"
#include "mw3gf/game/remote_control.hpp"
#include <algorithm>
#include <cmath>
namespace mw3gf::game {
bool GameplayController::gameplay_active() const noexcept {
    return context_.settings.gameplay && !context_.demo && foreground() &&
           context_.engine.read<std::uint8_t>(client_initialized_rva) != 0 &&
           context_.engine.read<int>(connection_state_rva) == 6 &&
           context_.engine.read<int>(key_capture_rva) == 0;
}
void GameplayController::request_release() noexcept {
    release_pending_.store(true, std::memory_order_release);
}
void GameplayController::pump_gameplay(bool running) noexcept {
    try {
        if (!running) {
            (void)cinematic_skip_.update({}, false);
            if (release_pending_.exchange(false))
                dispatch_gameplay(gameplay_adapter_.update({}, false));
            return;
        }
        if (!game_frame_seen_) {
            context_.logger.write("Native client frame observed; gameplay input pump active.");
            game_frame_seen_ = true;
        }
        context_.input.poll_gameplay();
        const auto& input = context_.input.monitor().snapshot();
        pump_cinematic(input);
        auto* font = context_.engine.read<NativeFont*>(normal_font_pointer_rva);
        prompts_.prepare(font, input.connected);
        prompts_.update_mode(input.connected);
        movement_ = gameplay_adapter_.update(input, gameplay_active());
        if (movement_.reset_camera)
            camera_.reset();
        if (gameplay_active())
            (void)context_.menu_adapter.update(input, {false, 0, 0}, GetTickCount64());
        dispatch_gameplay(movement_);
        gameplay_error_logged_ = false;
    } catch (const std::exception& e) {
        release_gameplay();
        if (!gameplay_error_logged_) {
            context_.logger.write(e.what());
            gameplay_error_logged_ = true;
        }
    } catch (...) {
        release_gameplay();
    }
}
void GameplayController::pump_cinematic(const InputSnapshot& input) {
    const int state = context_.engine.read<int>(connection_state_rva);
    const bool active = context_.settings.navigation && !context_.demo && foreground() &&
                        (state == cinematic_connection_state || state == logo_connection_state) &&
                        context_.engine.read<int>(key_capture_rva) == 0;
    if (active) {
        // Keep held A blocked when skipping opens the first menu.
        (void)context_.menu_adapter.update(input, {false, 0, 0}, GetTickCount64());
    }
    if (cinematic_skip_.update(input, active)) {
        context_.engine.function<void (*)(int)>(cinematic_escape_rva)(0);
        context_.logger.write("Gamepad A requested cinematic skip through native Escape handler.");
    }
}
void GameplayController::add_movement(void* cmd, int local_client) noexcept {
    if (local_client != 0 || !cmd || !gameplay_active() || !context_.input.monitor().snapshot().connected)
        return;
    // Match the native KeyboardMove early-out for frozen movement.
    if (context_.engine.read<unsigned>(client_state_rva + layout::client::movement_flags) &
        layout::client::frozen_movement)
        return;
    auto* bytes = static_cast<std::int8_t*>(cmd);
    bytes[layout::command::forward] = merge_movement(bytes[layout::command::forward], movement_.forward);
    bytes[layout::command::right] = merge_movement(bytes[layout::command::right], movement_.right);
    if (!game_command_seen_) {
        context_.logger.write("Native usercmd observed: Xbox squared forward/right applied at +1C/+1D.");
        game_command_seen_ = true;
    }
}
void GameplayController::add_camera(void* command, float seconds) noexcept {
    if (!context_.settings.camera || !command || !gameplay_active() ||
        !context_.input.monitor().snapshot().connected) {
        camera_.reset();
        return;
    }
    const auto* client = context_.engine.address(client_state_rva);
    // Native MouseMove and Xbox GamepadMove freeze flags, before any angles change.
    if ((read_field<unsigned>(client, layout::client::movement_flags) & layout::client::frozen_movement) ||
        (read_field<unsigned>(client, layout::client::camera_flags) & layout::client::frozen_camera)) {
        camera_.reset();
        return;
    }
    if (remote_camera_) {
        camera_.reset();
        remote_camera_ = false;
    }
    const auto* state = context_.engine.address(camera_state_rva);
    CameraContext context;
    context.seconds = seconds; // The original builder supplies scaled game seconds.
    if (read_field<std::uint8_t>(state, layout::camera::initialized)) {
        context.fov_scale = read_field<float>(state, layout::camera::fov_scale);
        context.ads = read_field<float>(state, layout::camera::ads);
        const auto buttons = read_field<unsigned>(command, layout::command::buttons);
        const auto force_ads = context_.engine.function<bool (*)(int)>(forced_ads_rva);
        if (((read_field<unsigned>(state, layout::camera::flags) & layout::camera::forced_ads_modes) &&
             (buttons & layout::command::ads_button)) ||
            force_ads(0))
            context.ads = 1;
    }
    context.pitch_cap = read_field<float>(client, layout::client::pitch_cap);
    context.yaw_cap = read_field<float>(client, layout::client::yaw_cap);
    auto* angles = static_cast<float*>(context_.engine.address(client_state_rva + layout::client::angles));
    if (!std::isfinite(angles[0]) || !std::isfinite(angles[1])) {
        camera_.reset();
        return;
    }
    const auto delta = camera_.update(movement_.look, context, context_.settings.camera_settings);
    angles[0] += delta.pitch;
    angles[1] += delta.yaw;
    // PC special modes consume raw analog look in +20/+21, with mouse signs.
    // +3E/+3F belongs to RemoteControlMove and is handled separately.
    const auto modes = read_field<unsigned>(client, layout::client::special_modes);
    if (modes & layout::client::analog_look_modes) {
        auto* bytes = static_cast<std::int8_t*>(command);
        const float pitch = context_.settings.camera_settings.invert_y ? movement_.look.y : -movement_.look.y;
        bytes[layout::command::pitch] =
            merge_movement(bytes[layout::command::pitch], static_cast<std::int8_t>(std::trunc(pitch * 127)));
        bytes[layout::command::yaw] = merge_movement(
            bytes[layout::command::yaw], static_cast<std::int8_t>(std::trunc(movement_.look.x * 127)));
    }
    if (!camera_seen_ && (delta.pitch || delta.yaw)) {
        try {
            context_.logger.write("Stage 3 camera active: native seconds, Xbox graph 3/rate ramp, native "
                                  "ADS/FOV/caps; float angles before packing.");
        } catch (...) {
        }
        camera_seen_ = true;
    }
}
void GameplayController::add_remote_stick(int local_client, void* command) noexcept {
    camera_.reset();
    remote_camera_ = true;
    if (!context_.settings.camera || local_client != 0 || !command || !gameplay_active() ||
        !context_.input.monitor().snapshot().connected)
        return;
    if (context_.engine.read<unsigned>(client_state_rva + layout::client::movement_flags) &
        layout::client::frozen_movement)
        return;
    add_remote_control(command, movement_.move, movement_.look, context_.settings.camera_settings.invert_y);
    if (!remote_seen_ && (movement_.look.x || movement_.look.y || movement_.move.x || movement_.move.y)) {
        try {
            context_.logger.write(
                "Remote analog input active: Xbox pitch +3E/yaw +3F, both sticks; ordinary camera bypassed.");
        } catch (...) {
        }
        remote_seen_ = true;
    }
}
void GameplayController::release_gameplay() noexcept {
    prompts_.update_mode(false);
    movement_ = {};
    camera_.reset();
    try {
        dispatch_gameplay(gameplay_adapter_.update({}, false));
    } catch (...) {
    }
}
void GameplayController::dispatch_gameplay(const GameplayOutput& commands) {
    using ExecBinding = void (*)(int, int, int);
    const auto execute = context_.engine.function<ExecBinding>(exec_binding_rva);
    for (std::size_t i = 0; i < commands.count; ++i) {
        const auto event = commands.events[i];
        const auto& binding = gameplay_bindings[event.binding];
        if (binding.control == bit(Button::start)) {
            if (event.down && gameplay_active()) {
                // PC dispatcher entries 63/64/65 are no-ops. Use the very
                // same UI activation as CL_KeyEvent(Escape) in CA_ACTIVE.
                context_.engine.function<void (*)(int, int)>(ui_activate_rva)(0, 2);
                context_.logger.write("Gamepad opened native pause/objectives menu.");
            }
            continue;
        }
        if (event.down && !gameplay_active())
            continue;
        const int action = event.down ? binding.down : binding.up;
        // Native KButton has two owner slots; this nonzero owner is outside
        // keyboard codes, and never indexes the keyboard state array.
        execute(0, action, 0x4000 + static_cast<int>(event.binding));
        context_.logger.write("Gameplay action=" + std::to_string(action) + (event.down ? " down" : " up"));
    }
}
} // namespace mw3gf::game
