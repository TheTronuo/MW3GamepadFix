#pragma once
#include "mw3gf/game/gameplay_controller.hpp"
#include "mw3gf/game/hook_set.hpp"
#include "mw3gf/game/menu_controller.hpp"
namespace mw3gf::game {
class ModRuntime {
  public:
    ModRuntime(HMODULE module, const plugin::StartOptions& options, OriginalFunctions& originals)
        : context_(module, options, originals), prompts_(context_, module), gameplay_(context_, prompts_),
          menu_(context_, prompts_) {}
    RuntimeContext& context() noexcept { return context_; }
    PromptController& prompts() noexcept { return prompts_; }
    GameplayController& gameplay() noexcept { return gameplay_; }
    MenuController& menu() noexcept { return menu_; }
    HookSet& hooks() noexcept { return hooks_; }

  private:
    RuntimeContext context_;
    PromptController prompts_;
    GameplayController gameplay_;
    MenuController menu_;
    HookSet hooks_;
};
} // namespace mw3gf::game
