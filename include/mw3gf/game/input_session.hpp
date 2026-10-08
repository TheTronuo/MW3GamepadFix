#pragma once
#include "mw3gf/core/input_monitor.hpp"
#include "mw3gf/game/runtime_log.hpp"
#include "mw3gf/input/backend.hpp"
#include "mw3gf/plugin/api.hpp"
namespace mw3gf::game {
class InputSession {
  public:
    InputSession(const plugin::StartOptions& options, RuntimeLog& log);
    void poll_gameplay();
    void poll_menu();
    [[nodiscard]] const InputMonitor& monitor() const noexcept { return monitor_; }

  private:
    std::unique_ptr<InputBackend> backend_;
    InputMonitor monitor_;
    bool demo_;
};
} // namespace mw3gf::game
