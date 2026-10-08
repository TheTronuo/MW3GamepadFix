#pragma once
#include "mw3gf/plugin/api.hpp"
#include <Windows.h>
namespace mw3gf::game {
// Called by an exported function after LoadLibrary completes, outside the loader lock.
void start(HMODULE plugin_module, const plugin::StartOptions& options);
void stop();
// Process-detach only: leave pinned GPU/queued-render storage to OS teardown.
void abandon_for_process_exit() noexcept;
} // namespace mw3gf::game
