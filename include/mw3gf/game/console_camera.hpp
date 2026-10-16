#pragma once
#include "mw3gf/core/console_aim.hpp"
#include "mw3gf/game/engine_api.hpp"
namespace mw3gf::game {
// Same input/output ABI as the native PC aim wrapper. Pointer-free, 32-bit fields.
struct NativeAimInput {
    float seconds{}, lock_seconds{}, pitch{}, pitch_axis{}, pitch_cap{};
    float yaw{}, yaw_axis{}, yaw_cap{}, forward{}, side{};
    unsigned buttons{};
    int local_client{};
};
struct NativeAimOutput {
    float pitch{}, yaw{}, charge_yaw{};
    std::uint8_t charge_distance{};
};
static_assert(sizeof(NativeAimInput) == 0x30);
static_assert(offsetof(NativeAimInput, buttons) == 0x28);
class ConsoleCamera {
  public:
    void reset() noexcept { camera_.reset(); aim_.reset(); }
    void update(EngineApi& engine, const NativeAimInput& input, NativeAimOutput& output,
                Stick look, Stick move, const CameraSettings& settings, bool assist,
                unsigned controller_buttons) noexcept;
  private:
    StickCamera camera_;
    ConsoleAim aim_;
};
} // namespace mw3gf::game
