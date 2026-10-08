#pragma once

#include "mw3gf/game/build_profile.hpp"

namespace mw3gf::game {

using RunMenuScript = void (*)(void* context, void* item, void* script);

class NativeMenuActions {
  public:
    NativeMenuActions(void* context, ActiveMenu active_menu, UiKeyEvent key_event, RunMenuScript run_script)
        : context_(context), active_menu_(active_menu), key_event_(key_event), run_script_(run_script) {}

    void press_key(int key) const;
    void run_script(void* item, void* script) const;

  private:
    void* context_;
    ActiveMenu active_menu_;
    UiKeyEvent key_event_;
    RunMenuScript run_script_;
};

} // namespace mw3gf::game
