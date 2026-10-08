#pragma once
#include "mw3gf/core/pad_state.hpp"
#include <chrono>
#include <deque>

namespace mw3gf {
struct InputEvent {
    std::wstring text;
};
// Only this class derives press/release edges. Rendering never mutates input state.
class InputMonitor {
  public:
    void update(InputSnapshot snapshot);
    [[nodiscard]] const InputSnapshot& snapshot() const noexcept { return snapshot_; }
    [[nodiscard]] const std::deque<InputEvent>& events() const noexcept { return events_; }

  private:
    void append(std::wstring text);
    void edges(std::uint32_t before, std::uint32_t after);
    InputSnapshot snapshot_;
    std::deque<InputEvent> events_;
    bool initialized_{};
};
} // namespace mw3gf
