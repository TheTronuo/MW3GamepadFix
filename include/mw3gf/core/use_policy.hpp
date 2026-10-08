#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
namespace mw3gf {
enum class UseAction { ground_weapon, throwing_knife, intelligence, breach };
struct UseTarget {
    unsigned entity_type{};
    std::uint32_t weapon{};
    int weapon_class{-1};
    std::string_view hint_key;
};
struct UseHoldRule {
    UseAction action;
    std::uint32_t hold_ms;
    unsigned entity_types;
    bool requires_weapon;
    int weapon_class; // -1 accepts any class.
    std::string_view hint_key;
    std::array<std::string_view, 2> prompt_keys;
    std::array<std::string_view, 2> press_reference_keys;
    bool allow_implicit_prefix{}; // Audited prompts may omit the Press instruction.
};
// One catalog controls both eligibility and prompt conversion. Ground weapon
// pickup and swapping share a native item route and therefore one rule.
[[nodiscard]] std::span<const UseHoldRule> use_hold_rules() noexcept;
[[nodiscard]] std::string_view use_hint_key(std::string_view key) noexcept;
[[nodiscard]] const UseHoldRule* find_use_hold_rule(const UseTarget& target) noexcept;
[[nodiscard]] const UseHoldRule* find_use_hold_prompt(std::string_view key) noexcept;
inline constexpr std::array<std::string_view, 2> use_hold_reference_keys{
    "PLATFORM_HOLD_TO_PLANT_EXPLOSIVES", "PLATFORM_HOLD_TO_DEFUSE_EXPLOSIVES"};
struct HoldTextTemplates {
    std::array<std::string_view, 2> press;
    std::array<std::string_view, 2> hold;
    bool allow_implicit_prefix{};
};
// Copy a complete shared instruction from native localized templates. The
// action and its button slot remain the target's; no translated words are read.
[[nodiscard]] std::string localized_hold_instruction(std::string_view original,
                                                      const HoldTextTemplates& templates);
} // namespace mw3gf
