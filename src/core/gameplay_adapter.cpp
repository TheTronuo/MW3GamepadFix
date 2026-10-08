#include "mw3gf/core/gameplay_adapter.hpp"
#include "mw3gf/core/stick_camera.hpp"
#include <algorithm>
#include <cmath>
namespace mw3gf {
GameplayOutput GameplayAdapter::update(const InputSnapshot& input, bool active) {
    GameplayOutput output;
    std::uint32_t physical = input.connected ? input.pad.buttons : 0;
    if (input.connected && std::isfinite(input.pad.left_trigger) && input.pad.left_trigger > 0.13f)
        physical |= control_lt;
    if (input.connected && std::isfinite(input.pad.right_trigger) && input.pad.right_trigger > 0.13f)
        physical |= control_rt;
    active = active && input.connected;
    const bool changed = active != active_ || device_ != input.device_id || backend_ != input.backend;
    if (changed || !active) {
        for (std::size_t i = 0; i < gameplay_bindings.size(); ++i)
            if ((held_ & gameplay_bindings[i].control) && gameplay_bindings[i].up)
                output.events[output.count++] = {i, false};
        held_ = 0;
        blocked_ = physical;
        left_blocked_ = right_blocked_ = true;
        output.reset_camera = true;
        device_ = input.device_id;
        backend_ = input.backend;
        active_ = active;
    }
    if (!active)
        return output;
    blocked_ &= physical;
    const auto wanted = physical & ~blocked_;
    for (std::size_t i = 0; i < gameplay_bindings.size(); ++i) {
        const auto& b = gameplay_bindings[i];
        if (!((held_ ^ wanted) & b.control))
            continue;
        const bool down = (wanted & b.control) != 0;
        if (down || b.up)
            output.events[output.count++] = {i, down};
    }
    held_ = wanted;
    const auto left = xbox_stick(input.pad.left_stick), right = xbox_stick(input.pad.right_stick);
    if (!left.x && !left.y)
        left_blocked_ = false;
    if (!right.x && !right.y)
        right_blocked_ = false;
    if (!left_blocked_) {
        output.move = left;
        const auto move = xbox_movement(left);
        output.forward = static_cast<std::int8_t>(move.y);
        output.right = static_cast<std::int8_t>(move.x);
    }
    if (!right_blocked_)
        output.look = right;
    return output;
}
std::optional<unsigned> gameplay_glyph(std::string_view command) noexcept {
    if (command.starts_with('-'))
        command.remove_prefix(1);
    else if (command.starts_with('+'))
        command.remove_prefix(1);
    for (const auto& b : gameplay_bindings) {
        auto name = b.command;
        if (name.starts_with('+'))
            name.remove_prefix(1);
        if (name == command)
            return b.glyph;
    }
    if (command == "activate" || command == "reload" || command == "use" || command == "use_reload")
        return icon_code(ButtonIcon::x);
    if (command == "melee" || command == "changezoom")
        return icon_code(ButtonIcon::right_stick);
    if (command == "sprint" || command == "holdbreath" || command == "forward" || command == "back" ||
        command == "moveleft" || command == "moveright")
        return icon_code(ButtonIcon::left_stick);
    if (command == "ads" || command == "speed" || command == "toggleads" || command == "toggleads_throw" ||
        command == "leaveads")
        return icon_code(ButtonIcon::left_trigger);
    if (command == "prone" || command == "goprone" || command == "gocrouch" || command == "togglecrouch" ||
        command == "toggleprone" || command == "movedown")
        return icon_code(ButtonIcon::b);
    if (command == "throw")
        return icon_code(ButtonIcon::right_trigger);
    if (command == "weapprev")
        return icon_code(ButtonIcon::y);
    if (command == "pause" || command == "skip")
        return icon_code(ButtonIcon::start);
    if (command == "lookup" || command == "lookdown" || command == "left" || command == "right" ||
        command == "mlook" || command == "centerview")
        return icon_code(ButtonIcon::right_stick);
    return std::nullopt;
}
bool is_prompt_glyph(unsigned code) noexcept {
    return (code >= 1 && code <= 6) || (code >= 14 && code <= 23);
}
std::int8_t merge_movement(std::int8_t native, std::int8_t controller) noexcept {
    return static_cast<std::int8_t>(std::clamp(static_cast<int>(native) + controller, -127, 127));
}
} // namespace mw3gf
