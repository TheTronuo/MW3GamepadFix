#pragma once
#include "mw3gf/core/pad_state.hpp"
namespace mw3gf {
// Xbox hardware normalization, radial deadzone and signed virtual-axis storage.
Stick xbox_stick(Stick raw) noexcept;
Stick xbox_movement(Stick processed) noexcept;
Stick xbox_view_curve(Stick processed) noexcept;
struct CameraSettings {
    float sensitivity{1.0f};
    bool invert_y{};
};
struct CameraContext {
    float seconds{}, ads{}, fov_scale{1.0f}, pitch_cap{}, yaw_cap{};
};
struct CameraDelta {
    float pitch{}, yaw{};
};
class StickCamera {
  public:
    CameraDelta update(Stick processed, const CameraContext& context,
                       const CameraSettings& settings) noexcept;
    void reset() noexcept { pitch_speed_ = yaw_speed_ = 0; }

  private:
    float pitch_speed_{}, yaw_speed_{};
};
} // namespace mw3gf
