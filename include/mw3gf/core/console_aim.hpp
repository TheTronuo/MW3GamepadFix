#pragma once
#include "mw3gf/core/stick_camera.hpp"
#include <array>
#include <cstdint>
#include <span>
namespace mw3gf {
using Vec3 = std::array<float, 3>;
struct AimTarget {
    int entity{0x7ff};
    std::array<float, 2> minimum{}, maximum{};
    Vec3 position{}, velocity{};
    float distance_squared{}, crosshair_distance_squared{};
};
static_assert(sizeof(AimTarget) == 0x34);
struct AimFrame {
    std::uint32_t flags{}, movement_flags{}, weapon_flags{}, buttons{}, previous_buttons{};
    int weapon_state{}, weapon_type{}, melee_phase{};
    bool weapon_valid{}, has_ammo{}, offhand{}, skip_pitch{}, scripted_slowdown{}, enabled{true};
    float pitch_scale{1}, yaw_scale{1}, ads{}, seconds{}, lock_seconds{};
    float hip_range{}, ads_range{}, auto_range{};
    std::array<float, 2> slowdown_region{}, auto_region{}, melee_region{}, lock_region{};
    Vec3 eye{}, velocity{}, view_angles{}, right{}, up{};
    std::span<const AimTarget> targets;
};
// PPC 820C27E8: shortest signed angle, including the +180 -> -180 boundary.
float console_angle_delta(float to, float from) noexcept;
float console_track_angle(float goal, float current, float seconds) noexcept;
struct ViewCone {
    bool constrained{}, pitch{};
    float angle{}, center{}, range{180}, positive_zone{}, negative_zone{};
    float pitch_up{85}, pitch_down{85};
};
float console_view_resistance(const ViewCone& cone, float input) noexcept;
const AimTarget* console_target(std::span<const AimTarget> targets, float range,
                                std::array<float, 2> region, int previous = 0x7ff) noexcept;
std::array<float, 2> console_slowdown(const AimFrame& frame) noexcept;
class ConsoleAim {
  public:
    void reset() noexcept;
    CameraDelta auto_aim(const AimFrame& frame) noexcept;
    CameraDelta lock_on(const AimFrame& frame, Stick look, Stick move) noexcept;
    bool auto_aim_active() const noexcept { return active_; }
  private:
    bool ads_pressed_{}, active_{};
    int auto_target_{0x7ff}, lock_target_{0x7ff};
    std::array<float, 2> current_{}, goal_{};
};
} // namespace mw3gf
