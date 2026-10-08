#pragma once
#include "mw3gf/core/cinematic_skip.hpp"
#include "mw3gf/core/gameplay_adapter.hpp"
#include "mw3gf/game/button_icons.hpp"
#include "mw3gf/game/prompt_controller.hpp"
#include "mw3gf/game/runtime_context.hpp"
#include <atomic>
namespace mw3gf::game {
class GameplayController {
  public:
    GameplayController(RuntimeContext& context, PromptController& prompts)
        : context_(context), prompts_(prompts) {}
    bool gameplay_active() const noexcept;
    void request_release() noexcept;
    void pump_gameplay(bool running) noexcept;
    void add_movement(void* cmd, int local_client) noexcept;
    void add_camera(void* command, float seconds) noexcept;
    void add_remote_stick(int local_client, void* command) noexcept;
    void record_use_command(const void* command, int local_client) noexcept;
    bool defer_use(const void* player, const void* target, std::uintptr_t caller_rva) noexcept;

  private:
    void pump_cinematic(const InputSnapshot& input);
    void release_gameplay() noexcept;
    void dispatch_gameplay(const GameplayOutput& commands);
    RuntimeContext& context_;
    PromptController& prompts_;
    GameplayAdapter gameplay_adapter_;
    CinematicSkip cinematic_skip_;
    GameplayOutput movement_;
    StickCamera camera_;
    std::atomic_bool release_pending_{};
    std::atomic_bool controller_use_source_{};
    UseHold use_hold_;
    bool camera_seen_{}, remote_seen_{}, remote_camera_{};
    bool game_command_seen_{}, gameplay_error_logged_{}, game_frame_seen_{};
};
} // namespace mw3gf::game
