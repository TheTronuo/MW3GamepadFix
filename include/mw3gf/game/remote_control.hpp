#pragma once
#include "mw3gf/core/pad_state.hpp"
namespace mw3gf::game {
// Processed sticks from GameplayAdapter, before the ordinary view curve.
void add_remote_control(void* command, Stick move, Stick look, bool invert_y) noexcept;
} // namespace mw3gf::game
