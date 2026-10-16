#include "mw3gf/core/console_aim.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace mw3gf {
namespace {
bool overlaps(const AimTarget& target, std::array<float, 2> region) noexcept {
    return target.minimum[0] <= region[0] && target.maximum[0] >= -region[0] &&
           target.minimum[1] <= region[1] && target.maximum[1] >= -region[1];
}
const AimTarget* by_id(std::span<const AimTarget> targets, int id) noexcept {
    for (const auto& target : targets)
        if (target.entity == id) return &target;
    return nullptr;
}
std::array<float, 2> target_angles(const AimTarget& target, const Vec3& eye) noexcept {
    const float x = target.position[0] - eye[0], y = target.position[1] - eye[1];
    const float z = target.position[2] - eye[2];
    constexpr float degrees = 180 / std::numbers::pi_v<float>;
    return {-std::atan2(z, std::hypot(x, y)) * degrees, std::atan2(y, x) * degrees};
}
bool valid_time(float value) noexcept { return std::isfinite(value) && value > 0 && value <= 1; }
} // namespace
float console_track_angle(float goal, float current, float seconds) noexcept {
    // 822E6B28: exponential approach at 40/s, without overshooting at low FPS.
    const float error = console_angle_delta(goal, current);
    if (std::abs(error) > .001f && std::abs(error * 40 * seconds) <= std::abs(error))
        goal = current + error * 40 * seconds;
    return console_angle_delta(goal, 0);
}
float console_angle_delta(float to, float from) noexcept {
    const float turns = (to - from) * (1.0f / 360);
    return (turns - std::floor(turns + .5f)) * 360;
}
float console_view_resistance(const ViewCone& cone, float input) noexcept {
    if (!cone.constrained || !std::isfinite(cone.range) || cone.range >= 180 ||
        !std::isfinite(input) || std::abs(input) <= .01f) return 1;
    float boundary = console_angle_delta(cone.center + (input > 0 ? cone.range : -cone.range), 0);
    if (cone.pitch && (boundary > cone.pitch_down || boundary < -cone.pitch_up))
        boundary = input > 0 ? cone.pitch_down : -cone.pitch_up;
    const float remaining = std::max(input > 0 ? console_angle_delta(boundary, cone.angle)
                                               : console_angle_delta(cone.angle, boundary), 0.0f);
    const float zone = console_angle_delta(input > 0 ? cone.positive_zone : cone.negative_zone, 0);
    if (!std::isfinite(remaining) || !std::isfinite(zone) || zone <= 0) return 1;
    return remaining < zone ? std::clamp(remaining / zone, 0.0f, 1.0f) : 1;
}
const AimTarget* console_target(std::span<const AimTarget> targets, float range,
                               std::array<float, 2> region, int previous) noexcept {
    if (!std::isfinite(range) || range <= 0 || !std::isfinite(region[0]) ||
        !std::isfinite(region[1]) || region[0] <= 0 || region[1] <= 0) return nullptr;
    const float squared = range * range;
    // Xbox 820C3CD8 gives the previous lock target priority, with a strict range test.
    if (previous != 0x7ff)
        if (const auto* old = by_id(targets, previous);
            old && old->distance_squared < squared && overlaps(*old, region)) return old;
    for (const auto& target : targets)
        if (target.distance_squared <= squared && overlaps(target, region)) return &target;
    return nullptr;
}
std::array<float, 2> console_slowdown(const AimFrame& frame) noexcept {
    // Script slowaim takes priority even when ordinary target assist is disabled.
    if (frame.scripted_slowdown) return {frame.pitch_scale, frame.yaw_scale};
    std::array<float, 2> result{1, 1};
    if (!frame.enabled || (frame.movement_flags & 4) || frame.weapon_state == 26 ||
        frame.weapon_state == 27 || !frame.weapon_valid ||
        (!(frame.flags & 0x201800) && !frame.has_ammo)) return result;
    const float range = frame.hip_range * (1 - frame.ads) + frame.ads_range * frame.ads;
    if (console_target(frame.targets, range, frame.slowdown_region))
        result.fill(.4f + frame.ads * .099999994f);
    if (frame.skip_pitch) result[0] = 1;
    return result;
}
void ConsoleAim::reset() noexcept {
    ads_pressed_ = active_ = false;
    auto_target_ = lock_target_ = 0x7ff;
    current_ = goal_ = {};
}
CameraDelta ConsoleAim::auto_aim(const AimFrame& frame) noexcept {
    if (!valid_time(frame.seconds) || !frame.enabled || !(frame.buttons & 0x800) ||
        frame.offhand || (frame.movement_flags & 4) || !frame.weapon_valid || frame.weapon_type == 4) {
        ads_pressed_ = active_ = false;
        auto_target_ = 0x7ff;
        current_ = goal_ = {};
        return {};
    }
    if (!ads_pressed_) {
        if (frame.weapon_state < 8 || frame.weapon_state > 12)
            if (const auto* target = console_target(frame.targets, frame.auto_range, frame.auto_region)) {
                auto_target_ = target->entity;
                current_ = {frame.view_angles[0], frame.view_angles[1]};
                goal_ = target_angles(*target, frame.eye);
                active_ = true;
            }
        ads_pressed_ = true;
    }
    if (!active_) return {};
    if (!(frame.flags & 0x1800) && frame.ads < 1) {
        const auto* target = by_id(frame.targets, auto_target_);
        if (!target) { active_ = false; auto_target_ = 0x7ff; return {}; }
        goal_ = target_angles(*target, frame.eye);
    }
    const std::array<float, 2> next{console_track_angle(goal_[0], current_[0], frame.seconds),
                                   console_track_angle(goal_[1], current_[1], frame.seconds)};
    const CameraDelta result{console_angle_delta(next[0], current_[0]),
                             console_angle_delta(next[1], current_[1])};
    current_ = next;
    if (std::abs(result.pitch) < .001f && std::abs(result.yaw) <= .001f) {
        active_ = false; auto_target_ = 0x7ff; current_ = goal_ = {};
    }
    return result;
}
CameraDelta ConsoleAim::lock_on(const AimFrame& frame, Stick look, Stick move) noexcept {
    const int previous = lock_target_;
    lock_target_ = 0x7ff;
    // Xbox examines pitch, yaw and side axis; forward alone does not engage lock-on.
    if (!valid_time(frame.lock_seconds) || !frame.enabled || frame.skip_pitch ||
        (frame.movement_flags & 4) || active_ || frame.melee_phase == 2 || !frame.weapon_valid ||
        (std::abs(look.x) < .05f && std::abs(look.y) < .05f && std::abs(move.x) < .05f)) return {};
    const float range = frame.hip_range * (1 - frame.ads) + frame.ads_range * frame.ads;
    const auto* target = console_target(frame.targets, range, frame.lock_region, previous);
    if (!target || target->distance_squared <= 0) return {};
    lock_target_ = target->entity;
    float side{}, up{};
    for (std::size_t i = 0; i < 3; ++i) {
        const float relative = target->velocity[i] - frame.velocity[i];
        side += relative * frame.right[i];
        up += relative * frame.up[i];
    }
    const float scale = 180 * .6f * frame.lock_seconds /
                        (std::numbers::pi_v<float> * std::sqrt(target->distance_squared));
    return {-up * scale, side * scale};
}
} // namespace mw3gf
