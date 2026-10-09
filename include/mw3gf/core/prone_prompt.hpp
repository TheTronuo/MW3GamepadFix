#pragma once
#include <array>
#include <string>
#include <string_view>
namespace mw3gf {
inline constexpr std::string_view prone_hold_reference_key = "SCRIPT_PLATFORM_HINT_HOLDDOWNPRONEKEY";
inline constexpr std::array<std::string_view, 10> prone_prompt_keys{
    "CASTLE_HINT_PRONE", "PLATFORM_STANCEHINT_PRONE", "PRAGUE_HINT_PRONE",
    "SCRIPT_PLATFORM_HINT_DOUBLETAPPRONEKEY", "SCRIPT_PLATFORM_HINT_HOLDDOWNPRONEKEY",
    "SCRIPT_PLATFORM_HINT_PRONEKEY", "WARLORD_HINT_PRONE", "WARLORD_HINT_PRONE_HOLD",
    "WARLORD_HINT_PRONE_STANCE", "WARLORD_HINT_PRONE_TOGGLE"};
[[nodiscard]] bool is_prone_prompt(std::string_view key) noexcept;
// Use the complete native Hold translation. Parameterized callers keep their
// &&1 slot so the engine still consumes the supplied argument; mission strings
// without arguments receive the controller's stance icon directly.
[[nodiscard]] std::string localized_prone_instruction(std::string_view key, std::string_view original,
                                                      std::string_view hold);
} // namespace mw3gf
