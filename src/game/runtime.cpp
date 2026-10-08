#include "mw3gf/game/runtime.hpp"
#include "native_hooks.hpp"
namespace mw3gf::game {
namespace detail {
// This pinned runtime deliberately has no CRT-owned destructor. Queued native
// commands retain its materials; GPU cleanup runs only at renderer teardown.
ModRuntime* runtime{};
OriginalFunctions originals{};
std::atomic_bool enabled{};
} // namespace detail
namespace {
std::mutex lifecycle_mutex;
}
using detail::enabled;
using detail::originals;
using detail::runtime;
void abandon_for_process_exit() noexcept {
    // CRT destroys globals under the loader lock. A D3D Release there can wait
    // on driver threads; queued native commands also retain these resources.
    // On process termination the OS reclaims them with the pinned module.
    enabled.store(false, std::memory_order_release);
}
void start(HMODULE module, const plugin::StartOptions& options) {
    std::scoped_lock lock(lifecycle_mutex);
    if (runtime)
        throw std::runtime_error(
            "Module already initialized; stop is supported, changing backend requires a game restart");
    auto instance = std::make_unique<ModRuntime>(module, options, originals);
    auto& hooks = instance->hooks();
    try {
        detail::install_hooks(*instance);
        detail::pin_hook_module();
        runtime = instance.get();
        enabled.store(true, std::memory_order_release);
        hooks.enable();
        hooks.retain_for_process_exit();
        (void)instance.release();
    } catch (...) {
        enabled.store(false, std::memory_order_release);
        runtime = nullptr;
        throw;
    }
    runtime->context().logger.write(
        "Native menu adapter enabled: A/Start=Enter, B=original Back/Quit; footer Y/X/Back=original "
        "callbacks. Mouse footer rendering/focus replaced only in controller mode.");
    runtime->context().logger.write("Stage 3 ready: Xbox movement/graph 3, stick camera and remote axes; "
                                    "native buttons/prompts retained. Back/View unassigned.");
}
void stop() {
    std::scoped_lock lock(lifecycle_mutex);
    if (!runtime || !enabled.exchange(false, std::memory_order_acq_rel))
        return;
    runtime->gameplay().request_release();
    // Keep the frame trampoline active: flush owned releases on the game
    // thread on its next frame, then it only forwards the original call.
    runtime->hooks().suspend_input();
    // Resources remain alive: a frame can still be returning through a detour.
}
} // namespace mw3gf::game
