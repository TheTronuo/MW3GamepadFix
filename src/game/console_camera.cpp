#include "mw3gf/game/console_camera.hpp"
#include "mw3gf/game/native_layout.hpp"
#include <algorithm>
#include <cmath>
namespace mw3gf::game {
namespace {
using Attachments = void (*)(unsigned, bool, void**, void**, void**);
// Only use inside ranges validated for this call. The engine's static camera
// and predicted-player blocks remain allocated throughout the native aim call.
// Dynamic weapon/attachment pointers keep their own readability checks.
template <class T> T read_validated(const void* object, std::size_t offset) noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    T value{};
    std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));
    return value;
}
template <class T> void store(void* object, std::size_t offset, const T& value) noexcept {
    std::memcpy(static_cast<std::byte*>(object) + offset, &value, sizeof(value));
}
void weapon_parameters(EngineApi& engine, const void* state, AimFrame& frame) noexcept {
    const unsigned weapon = read_validated<unsigned>(state, 0x24);
    frame.weapon_valid = (weapon & 0xff) != 0;
    if (!frame.weapon_valid) return;
    unsigned index = weapon & 0xff;
    const bool alternate = (frame.weapon_flags & 0x8000) != 0;
    // Same alternate-definition rule as native BG_GetEnemyCrosshairRange (RVA 25110).
    if (alternate) {
        const auto* complete = engine.read<void*>(0x4E86E0 + index * 8);
        const auto name = read_field<const char*>(complete, 0);
        const unsigned alt = read_field<unsigned>(complete, 0xC8) & 0xff;
        if (alt && readable(name, 4) && std::memcmp(name, "iw5_", 4) != 0) index = alt;
    }
    const auto* definition = engine.read<void*>(0x4E8E60 + index * 8);
    if (!readable(definition, 0x534)) { frame.weapon_valid = false; return; }
    frame.weapon_type = read_validated<int>(definition, 0x54);
    frame.auto_range = read_validated<float>(definition, 0x520);
    frame.hip_range = read_validated<float>(definition, 0x524);
    frame.ads_range = read_validated<float>(definition, 0x528);
    std::array<void*, 3> attachments{};
    engine.function<Attachments>(0x26B50)(weapon, alternate, &attachments[0], &attachments[1],
                                        &attachments[2]);
    bool range_override{}, type_override{};
    for (const auto* attachment : attachments) {
        if (!attachment) continue;
        if (!type_override) {
            const int type = read_field<int>(attachment, 0x14);
            if (type) { frame.weapon_type = type; type_override = true; }
        }
        const auto* ranges = read_field<void*>(attachment, 0x60);
        if (!range_override && readable(ranges, 12)) {
            frame.auto_range = read_validated<float>(ranges, 0);
            frame.hip_range = read_validated<float>(ranges, 4);
            frame.ads_range = read_validated<float>(ranges, 8);
            range_override = true;
        }
    }
    // 820C3E90 / 8235F798: ADS-only weapon modes can exempt pitch slowdown/lock-on.
    frame.skip_pitch = (frame.weapon_flags & 2) &&
        engine.function<WeaponClass>(weapon_class_rva)(weapon, false) != 9;
}
AimFrame frame_data(EngineApi& engine, const void* state, const NativeAimInput& input,
                    std::array<AimTarget, 64>& targets, bool assist) noexcept {
    AimFrame frame;
    frame.enabled = assist;
    frame.flags = read_validated<unsigned>(state, 0xC);
    frame.movement_flags = read_validated<unsigned>(state, 0x10);
    frame.weapon_flags = read_validated<unsigned>(state, 0x18);
    frame.weapon_state = read_validated<int>(state, 0x1C);
    frame.has_ammo = read_validated<std::uint8_t>(state, 0x28) != 0;
    frame.offhand = read_validated<std::uint8_t>(state, 0x29) != 0;
    frame.melee_phase = read_validated<int>(state, 0xE44);
    frame.buttons = input.buttons;
    frame.previous_buttons = read_validated<unsigned>(state, 0xB4);
    frame.seconds = input.seconds;
    frame.lock_seconds = input.lock_seconds;
    frame.ads = std::clamp(read_validated<float>(state, 0x120), 0.0f, 1.0f);
    frame.scripted_slowdown = read_validated<std::uint8_t>(state, 0xE38) != 0;
    frame.pitch_scale = read_validated<float>(state, 0xE3C);
    frame.yaw_scale = read_validated<float>(state, 0xE40);
    frame.velocity = read_validated<Vec3>(state, 0);
    frame.eye = read_validated<Vec3>(state, 0xDC);
    frame.view_angles = read_validated<Vec3>(state, 0xE8);
    frame.right = read_validated<Vec3>(state, 0x100);
    frame.up = read_validated<Vec3>(state, 0x10C);
    const auto* dimensions = engine.function<const float* (*)(int)>(0x8D920)(input.local_client);
    const float width = read_field<float>(dimensions, 0), height = read_field<float>(dimensions, 4);
    const float pixels_x = read_validated<float>(state, 0x12C), pixels_y = read_validated<float>(state, 0x130);
    if (pixels_x > 0 && pixels_y > 0) {
        const float inverse_fov = read_validated<float>(state, 0x11C);
        const float x = width / pixels_x, y = height / pixels_y;
        frame.slowdown_region = {90 * x, 90 * y};
        frame.auto_region = {160 * x * inverse_fov, 120 * y * inverse_fov};
        frame.melee_region = {320 * x * inverse_fov, 240 * y * inverse_fov};
        frame.lock_region = frame.slowdown_region;
    }
    const int count = std::clamp(read_validated<int>(state, 0xE34), 0, 64);
    std::size_t valid{};
    for (int i = 0; i < count; ++i) {
        const auto target = read_validated<AimTarget>(state, 0x134 + static_cast<std::size_t>(i) * 0x34);
        // The native list is already visibility-filtered and sorted by crosshair distance.
        // Reject bad snapshots before doing divisions, without inventing additional targeting rules.
        bool finite = std::isfinite(target.distance_squared) && target.distance_squared >= 0;
        for (float value : target.position) finite &= std::isfinite(value);
        for (float value : target.velocity) finite &= std::isfinite(value);
        for (float value : target.minimum) finite &= std::isfinite(value);
        for (float value : target.maximum) finite &= std::isfinite(value);
        if (finite && target.entity >= 0 && target.entity < 0x7ff) targets[valid++] = target;
    }
    frame.targets = {targets.data(), valid};
    weapon_parameters(engine, state, frame);
    return frame;
}
float resistance(EngineApi& engine, int axis, float input) noexcept {
    // Xbox 820C2878, playerState layout independently mapped to PC predicted state.
    const auto* ps = engine.address(client_state_rva + 0x2088);
    if (!readable(ps, 0x14C)) return 1;
    const unsigned other = read_validated<unsigned>(ps, 0x14), flags = read_validated<unsigned>(ps, 0xAC);
    const bool constrained = (read_validated<int>(ps, 4) == 1 && !(other & 1)) ||
                             (other & 4) || (flags & 0x1800);
    if (!constrained) return 1;
    const auto offset = static_cast<std::size_t>(axis) * 4;
    const auto* up = engine.read<void*>(0x4E7FE8);
    const auto* down = engine.read<void*>(0x4E7FF0);
    return console_view_resistance({constrained, axis == 0,
        read_validated<float>(ps, 0x10C + offset), read_validated<float>(ps, 0x12C + offset),
        read_validated<float>(ps, 0x134 + offset), read_validated<float>(ps, 0x144 + offset),
        read_validated<float>(ps, 0x13C + offset), read_field<float>(up, 0x10),
        read_field<float>(down, 0x10)}, input);
}
void melee(EngineApi& engine, void* state, const NativeAimInput& input, NativeAimOutput& output,
           const AimFrame& frame) noexcept {
    // Preserve PC native charge/tag lookup and angle interpolation. Restore the missing
    // Xbox previous-target selection and its larger, FOV-scaled melee region at acquisition.
    auto copied = input;
    bool acquired{};
    float charge_yaw{};
    std::uint8_t charge_distance{};
    if (frame.weapon_valid && frame.weapon_state != 26 && frame.weapon_state != 27 &&
        frame.melee_phase != 2 && !(frame.previous_buttons & 4) && (frame.buttons & 4)) {
        const AimTarget* target{};
        const int previous = read_validated<int>(state, 0xE48);
        for (const auto& entry : frame.targets)
            if (entry.entity == previous) { target = &entry; break; }
        if (!target) {
            const float range = read_validated<std::uint8_t>(state, 0x2A) ? 192.0f : 128.0f;
            target = console_target(frame.targets, range, frame.melee_region);
        }
        if (target) {
            const int phase = 1;
            const float zero{};
            store(state, 0xE44, phase); store(state, 0xE48, target->entity);
            store(state, 0xE4C, frame.view_angles[0]); store(state, 0xE50, zero);
            store(state, 0xE54, frame.view_angles[1]); store(state, 0xE58, zero);
            acquired = true;
            if (!(read_validated<unsigned>(state, 0x14) & 1) && target->distance_squared > 0) {
                const float distance = std::sqrt(target->distance_squared) - 30;
                if (distance > 0) {
                    charge_distance = static_cast<std::uint8_t>(std::min(distance, 255.0f));
                    const auto origin = read_validated<Vec3>(state, 0xD0);
                    charge_yaw = std::atan2(target->position[1] - origin[1],
                                            target->position[0] - origin[0]) * (180.0f / 3.14159265358979323846f);
                    if (charge_yaw < 0) charge_yaw += 360;
                }
            }
        } else {
            const int phase{}, none = 0x7ff;
            store(state, 0xE44, phase); store(state, 0xE48, none);
            for (std::size_t offset = 0xE4C; offset <= 0xE58; offset += 4) store(state, offset, 0.0f);
        }
        copied.buttons &= ~4U; // Selection has executed; native charge must not select again.
    }
    engine.originals().aim_update(&copied, &output);
    if (acquired) { output.charge_yaw = charge_yaw; output.charge_distance = charge_distance; }
}
} // namespace
void ConsoleCamera::update(EngineApi& engine, const NativeAimInput& input, NativeAimOutput& output,
                            Stick look, Stick move, const CameraSettings& settings, bool assist,
                            unsigned controller_buttons) noexcept {
    auto* state = engine.address(camera_state_rva);
    if (!readable(state, 0xE60) || !read_validated<std::uint8_t>(state, layout::camera::initialized) ||
        !std::isfinite(input.seconds) || input.seconds <= 0 || input.seconds > 1 ||
        !std::isfinite(output.pitch) || !std::isfinite(output.yaw)) {
        reset();
        engine.originals().aim_update(&input, &output);
        return;
    }
    std::array<AimTarget, 64> targets{};
    auto frame = frame_data(engine, state, input, targets, assist);
    // Native buttons merge keyboard and pad owners. Only pad ADS/melee may acquire Xbox targets.
    frame.buttons &= controller_buttons;
    bool finite = std::isfinite(frame.ads);
    for (const auto* vector : {&frame.eye, &frame.view_angles, &frame.velocity, &frame.right, &frame.up})
        for (float value : *vector) finite &= std::isfinite(value);
    if (!finite) {
        reset();
        engine.originals().aim_update(&input, &output);
        return;
    }
    const auto slow = console_slowdown(frame);
    CameraContext context{input.seconds, frame.ads, read_validated<float>(state, 0x118),
        input.pitch_cap, input.yaw_cap, slow[0], slow[1]};
    auto effective_look = frame.melee_phase == 2 ? Stick{} : look;
    const auto curved = xbox_view_curve(effective_look);
    context.pitch_resistance = resistance(engine, 0, settings.invert_y ? curved.y : -curved.y);
    context.yaw_resistance = resistance(engine, 1, -curved.x);
    const auto turn = camera_.update(effective_look, context, settings);
    output.pitch += turn.pitch; output.yaw += turn.yaw;
    const auto snap = aim_.auto_aim(frame);
    output.pitch += snap.pitch; output.yaw += snap.yaw;
    melee(engine, state, input, output, frame);
    frame.melee_phase = read_validated<int>(state, 0xE44);
    const auto follow = aim_.lock_on(frame, look, move);
    output.pitch += follow.pitch; output.yaw += follow.yaw;
}
} // namespace mw3gf::game
