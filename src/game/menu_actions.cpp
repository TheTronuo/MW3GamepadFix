#include "mw3gf/game/menu_actions.hpp"

namespace mw3gf::game {

void NativeMenuActions::press_key(int key) const {
    void* previous = active_menu_(context_);
    key_event_(0, key, 1);
    void* current = active_menu_(context_);
    // A key-up must not reach a different menu opened by this action. With no
    // remaining menu, the UI wrapper performs its normal close finalization.
    if (current == previous || !current) {
        key_event_(0, key, 0);
    }
}

void NativeMenuActions::run_script(void* item, void* script) const {
    run_script_(context_, item, script);
    if (!active_menu_(context_)) {
        // Mouse-footer callbacks bypass UI_KeyEvent's post-action cleanup.
        // Its no-menu branch releases the UI catcher, clears keys and resets
        // cl_paused. Key 0/up cannot activate anything when no menu remains.
        key_event_(0, 0, 0);
    }
}

} // namespace mw3gf::game
