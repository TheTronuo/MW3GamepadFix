#include "mw3gf/core/cinematic_skip.hpp"

namespace mw3gf {
bool CinematicSkip::update(const InputSnapshot& input, bool active) {
    active = active && input.connected;
    const bool held = input.connected && input.pad.held(Button::a);
    const bool continuing = active && active_ && device_ == input.device_id && backend_ == input.backend;
    const bool skip = continuing && held && !held_;
    active_ = active;
    held_ = held;
    device_ = input.device_id;
    backend_ = input.backend;
    return skip;
}
} // namespace mw3gf
