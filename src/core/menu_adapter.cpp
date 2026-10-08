#include "mw3gf/core/menu_adapter.hpp"
#include <cmath>

namespace mw3gf {
namespace {
constexpr auto confirm_mask = bit(Button::a) | bit(Button::start);
constexpr auto action_mask = confirm_mask | bit(Button::b) | bit(Button::x) | bit(Button::y) |
                             bit(Button::back) | bit(Button::lb) | bit(Button::rb) | bit(Button::ls) |
                             bit(Button::rs);
int axis(float value, int previous) noexcept {
    if (!std::isfinite(value))
        return 0;
    if (value >= 0.5f)
        return 1;
    if (value <= -0.5f)
        return -1;
    if (previous == 1 && value > 0.3f)
        return 1;
    if (previous == -1 && value < -0.3f)
        return -1;
    return 0;
}
} // namespace
int MenuAdapter::direction(const PadState& pad) noexcept {
    axis_x_ = axis(pad.left_stick.x, axis_x_);
    axis_y_ = axis(pad.left_stick.y, axis_y_);
    const int vertical = static_cast<int>(pad.held(Button::up)) - static_cast<int>(pad.held(Button::down));
    const int horizontal =
        static_cast<int>(pad.held(Button::right)) - static_cast<int>(pad.held(Button::left));
    const bool digital =
        (pad.buttons & (bit(Button::up) | bit(Button::down) | bit(Button::left) | bit(Button::right))) != 0;
    int x = digital ? horizontal : axis_x_, y = digital ? vertical : axis_y_;
    if (x && y) {
        // One focus transition per tick, including diagonals. Prefer vertical
        // for the D-pad and the dominant component for analog navigation.
        if (digital || std::abs(pad.left_stick.y) >= std::abs(pad.left_stick.x))
            x = 0;
        else
            y = 0;
    }
    if (y > 0)
        return 0;
    if (y < 0)
        return 1;
    if (x < 0)
        return 2;
    if (x > 0)
        return 3;
    return -1;
}
MenuActions MenuAdapter::update(const InputSnapshot& input, MenuContext context, std::uint64_t now) {
    MenuActions result;
    const bool active = input.connected && context.active;
    const auto buttons = input.connected ? input.pad.buttons & action_mask : 0u;
    const int current_direction = input.connected ? direction(input.pad) : -1;
    const bool changed =
        !active_ || menu_ != context.identity || device_ != input.device_id || backend_ != input.backend;
    if (!active || changed) {
        blocked_ = buttons;
        previous_ = buttons;
        direction_ = -1;
        block_direction_ = current_direction != -1;
        active_ = active;
        menu_ = context.identity;
        device_ = input.device_id;
        backend_ = input.backend;
        if (!active) {
            axis_x_ = 0;
            axis_y_ = 0;
        }
        return result;
    }
    blocked_ &= buttons;
    const auto pressed = buttons & ~previous_ & ~blocked_;
    previous_ = buttons;
    // Back takes precedence over simultaneous confirm to avoid activating
    // an item in a menu that the same sample closes.
    if (pressed & bit(Button::b))
        result.add(MenuAction::back);
    else if (pressed & confirm_mask)
        result.add(MenuAction::confirm);
    else if (pressed & context.shortcut_buttons & bit(Button::y))
        result.add(MenuAction::auxiliary_y);
    else if (pressed & context.shortcut_buttons & bit(Button::x))
        result.add(MenuAction::auxiliary_x);
    else if (pressed & context.shortcut_buttons & bit(Button::back))
        result.add(MenuAction::auxiliary_view);
    else if (pressed & context.shortcut_buttons & bit(Button::lb))
        result.add(MenuAction::page_up);
    else if (pressed & context.shortcut_buttons & bit(Button::rb))
        result.add(MenuAction::page_down);
    else if (pressed & context.shortcut_buttons & bit(Button::ls))
        result.add(MenuAction::top);
    else if (pressed & context.shortcut_buttons & bit(Button::rs))
        result.add(MenuAction::bottom);
    if (block_direction_) {
        if (current_direction == -1)
            block_direction_ = false;
        return result;
    }
    constexpr std::array directions{MenuAction::up, MenuAction::down, MenuAction::left, MenuAction::right};
    if (current_direction == -1) {
        direction_ = -1;
        return result;
    }
    if (current_direction != direction_) {
        result.add(directions[static_cast<std::size_t>(current_direction)]);
        repeat_at_ = now + 420;
    } else if (now >= repeat_at_) {
        result.add(directions[static_cast<std::size_t>(current_direction)]);
        repeat_at_ = now + 210; // Never accumulate a burst after a slow frame.
    }
    direction_ = current_direction;
    return result;
}
} // namespace mw3gf
