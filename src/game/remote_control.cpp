#include "mw3gf/game/remote_control.hpp"
#include "mw3gf/core/gameplay_adapter.hpp"
#include "mw3gf/game/native_layout.hpp"
#include <algorithm>
#include <cmath>
namespace mw3gf::game {
namespace {
Stick valid_stick(Stick stick) noexcept {
    if (!std::isfinite(stick.x) || !std::isfinite(stick.y))
        return {};
    return {std::clamp(stick.x, -1.0f, 1.0f), std::clamp(stick.y, -1.0f, 1.0f)};
}
std::int8_t remote_axis(float value) noexcept {
    return static_cast<std::int8_t>(std::floor(std::clamp(value, -1.0f, 1.0f) * 127 + .5f));
}
} // namespace
void add_remote_control(void* command, Stick move, Stick look, bool invert_y) noexcept {
    if (!command)
        return;
    move = valid_stick(move);
    look = valid_stick(look);
    // Xbox RemoteControlMove 0x8211F9B8: pitch = VA_PITCH + VA_FORWARD,
    // yaw = -VA_YAW - VA_SIDE. Default LS bindings use axis * pair radius;
    // RS bindings are linear. Normal pitch is negated, inverted pitch is not.
    const float radius = std::hypot(move.x, move.y);
    const float pitch = (look.y + move.y * radius) * (invert_y ? 1.0f : -1.0f);
    const float yaw = -look.x - move.x * radius;
    auto* bytes = static_cast<std::int8_t*>(command);
    bytes[layout::command::remote_pitch] =
        merge_movement(bytes[layout::command::remote_pitch], remote_axis(pitch));
    bytes[layout::command::remote_yaw] =
        merge_movement(bytes[layout::command::remote_yaw], remote_axis(yaw));
}
} // namespace mw3gf::game
