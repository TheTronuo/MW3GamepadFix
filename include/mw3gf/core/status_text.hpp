#pragma once
#include "mw3gf/core/pad_state.hpp"
namespace mw3gf {
[[nodiscard]] std::wstring pressed_controls(const InputSnapshot& snapshot);
}
