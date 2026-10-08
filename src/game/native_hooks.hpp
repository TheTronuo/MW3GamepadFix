#pragma once
#include "mw3gf/game/mod_runtime.hpp"
namespace mw3gf::game::detail {
extern ModRuntime* runtime;
extern OriginalFunctions originals;
extern std::atomic_bool enabled;
void install_hooks(ModRuntime& instance);
void pin_hook_module();
} // namespace mw3gf::game::detail
