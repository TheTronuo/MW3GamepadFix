#include "mw3gf/game/runtime_context.hpp"
#include "mw3gf/platform/windows.hpp"
#include <stdexcept>
namespace mw3gf::game {
RuntimeContext::RuntimeContext(HMODULE module, const plugin::StartOptions& options,
                               OriginalFunctions& originals)
    : engine(originals),
      settings(read_settings(win::module_path(module).parent_path() / L"MW3GamepadFix.ini")),
      logger(win::module_path(module).parent_path() / L"MW3GamepadFix.log"), input(options, logger),
      demo(options.demo != 0) {
    logger.write(settings.prompt_style == PromptStyle::ps3 ? "Prompt style: PS3 (independent icons)."
                                                           : "Prompt style: X360 (independent icons).");
    logger.write(
        "Build identity and paint/text/UI-key signatures verified; independent button resource validated.");
}
bool foreground() noexcept {
    DWORD process{};
    GetWindowThreadProcessId(GetForegroundWindow(), &process);
    return process == GetCurrentProcessId();
}
} // namespace mw3gf::game
