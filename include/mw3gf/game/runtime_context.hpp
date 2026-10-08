#pragma once
#include "mw3gf/core/input_monitor.hpp"
#include "mw3gf/core/menu_adapter.hpp"
#include "mw3gf/game/engine_api.hpp"
#include "mw3gf/game/input_session.hpp"
#include "mw3gf/game/runtime_log.hpp"
#include "mw3gf/game/settings.hpp"
#include "mw3gf/input/backend.hpp"
#include "mw3gf/plugin/api.hpp"
#include <Windows.h>
namespace mw3gf::game {
struct RuntimeContext {
    RuntimeContext(HMODULE module, const plugin::StartOptions& options, OriginalFunctions& originals);
    EngineApi engine;
    ModSettings settings;
    RuntimeLog logger;
    InputSession input;
    MenuAdapter menu_adapter;
    bool demo;
};
[[nodiscard]] bool foreground() noexcept;
} // namespace mw3gf::game
