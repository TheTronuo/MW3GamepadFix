#pragma once
#include "mw3gf/core/pad_state.hpp"

namespace mw3gf {
class CinematicSkip {
  public:
    [[nodiscard]] bool update(const InputSnapshot& input, bool active);

  private:
    bool active_{};
    bool held_{};
    std::wstring device_;
    std::wstring backend_;
};
} // namespace mw3gf
