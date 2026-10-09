#pragma once
#include "mw3gf/core/use_policy.hpp"
namespace mw3gf {
inline constexpr std::string_view mortar_prompt_key = "WARLORD_HINT_USE_MORTAR";
inline constexpr std::array<std::string_view, 2> mortar_press_reference_keys{
    "SCRIPT_PLATFORM_HINT_PRONEKEY", "SCRIPT_PLATFORM_HINT_CROUCHKEY"};
inline constexpr std::array<std::string_view, 2> mortar_hold_reference_keys{
    "SCRIPT_HOLD_TO_USE", "WEAPON_CLAYMORE_PICKUP"};
[[nodiscard]] bool is_mortar_prompt(std::string_view key) noexcept;
// This script-owned mortar uses immediate button edges. Transfer a complete
// localized Press instruction while preserving the mortar action and slot.
[[nodiscard]] std::string localized_mortar_instruction(std::string_view key, std::string_view original,
                                                       const HoldTextTemplates& templates);
} // namespace mw3gf
