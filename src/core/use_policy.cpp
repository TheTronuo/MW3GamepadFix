#include "mw3gf/core/use_policy.hpp"
#include <algorithm>
#include <optional>
namespace mw3gf {
namespace {
using namespace std::string_view_literals;
constexpr std::array weapon_press{"PLATFORM_SWAPWEAPONS"sv, "PLATFORM_PICKUPNEWWEAPON"sv};
constexpr std::array interaction_press{"SCRIPT_INTELLIGENCE_PICKUP"sv, "SCRIPT_PLATFORM_BREACH_ACTIVATE"sv};
constexpr unsigned hinted_entities = (1u << 0) | (1u << 5);
constexpr std::array rules{
    UseHoldRule{UseAction::ground_weapon, 250, 1u << 2, true, -1, {}, weapon_press, weapon_press, true},
    UseHoldRule{UseAction::throwing_knife, 250, 1u << 3, true, 9, {},
                {"PLATFORM_PICKUPNEWWEAPON", {}}, weapon_press, true},
    UseHoldRule{UseAction::intelligence, 250, hinted_entities, false, -1, "SCRIPT_INTELLIGENCE_PICKUP",
                {"SCRIPT_INTELLIGENCE_PICKUP", {}}, interaction_press},
    UseHoldRule{UseAction::breach, 250, hinted_entities, false, -1, "SCRIPT_PLATFORM_BREACH_ACTIVATE",
                {"SCRIPT_PLATFORM_BREACH_ACTIVATE", {}}, interaction_press}};
struct ButtonTemplate { std::string_view before, button, after; };
constexpr std::array slots{"[{+activate}]"sv, "[{+usereload}]"sv, "{+activate}"sv,
                           "{+usereload}"sv, "&&1"sv};
std::optional<ButtonTemplate> split_button(std::string_view text) {
    std::size_t at = std::string_view::npos;
    std::string_view slot;
    for (const auto candidate : slots) {
        const auto found = text.find(candidate);
        if (found < at) { at = found; slot = candidate; }
    }
    if (at == std::string_view::npos)
        return {};
    const auto before = text.substr(0, at), after = text.substr(at + slot.size());
    // Reject composite instructions and ambiguous parameters rather than
    // changing a sentence that contains more than one input.
    for (const auto part : {before, after})
        if (part.find('{') != std::string_view::npos || part.find("&&") != std::string_view::npos)
            return {};
    return ButtonTemplate{before, slot, after};
}
bool space(char c) noexcept { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
std::string instruction(std::string_view text) {
    std::string result;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '^' && i + 1 < text.size() && text[i + 1] >= '0' && text[i + 1] <= '9')
            ++i;
        else
            result += text[i];
    }
    const auto begin = std::find_if_not(result.begin(), result.end(), space);
    const auto end = std::find_if_not(result.rbegin(), result.rend(), space).base();
    return begin < end ? std::string(begin, end) : std::string{};
}
} // namespace
std::span<const UseHoldRule> use_hold_rules() noexcept { return rules; }
std::string_view use_hint_key(std::string_view key) noexcept {
    if (!key.empty() && (key.front() == '@' || key.front() == '\x14'))
        key.remove_prefix(1);
    return key;
}
const UseHoldRule* find_use_hold_rule(const UseTarget& target) noexcept {
    if (target.entity_type >= 32)
        return nullptr;
    for (const auto& rule : rules) {
        if (!(rule.entity_types & (1u << target.entity_type)) ||
            (rule.requires_weapon && !target.weapon) ||
            (rule.weapon_class >= 0 && rule.weapon_class != target.weapon_class) ||
            (!rule.hint_key.empty() && rule.hint_key != use_hint_key(target.hint_key)))
            continue;
        return &rule;
    }
    return nullptr;
}
const UseHoldRule* find_use_hold_prompt(std::string_view key) noexcept {
    key = use_hint_key(key);
    if (key.empty())
        return nullptr;
    for (const auto& rule : rules)
        for (const auto prompt : rule.prompt_keys)
            if (prompt == key)
                return &rule;
    return nullptr;
}
std::string localized_hold_instruction(std::string_view original, const HoldTextTemplates& templates) {
    const auto target = split_button(original);
    const auto press0 = split_button(templates.press[0]), press1 = split_button(templates.press[1]);
    const auto hold0 = split_button(templates.hold[0]), hold1 = split_button(templates.hold[1]);
    if (!target || !press0 || !press1 || !hold0 || !hold1)
        return std::string(original);
    const auto shared = [](std::string_view value, std::string_view a, std::string_view b,
                           std::string_view c, std::string_view d) {
        const auto from = instruction(a), to = instruction(c);
        return !from.empty() && !to.empty() && from == instruction(b) &&
               to == instruction(d) && instruction(value) == from;
    };
    bool prefix = shared(target->before, press0->before, press1->before, hold0->before, hold1->before);
    if (!prefix && templates.allow_implicit_prefix) {
        const auto a = instruction(press0->before), b = instruction(press1->before);
        const auto value = instruction(target->before), to = instruction(hold0->before);
        // The Russian weapon swap omits its instruction, while pickup has
        // one. Only audited keys can promote this mixed reference pair to
        // the common native Hold prefix. Never replace an action prefix in
        // languages whose instruction follows the button.
        prefix = a.empty() != b.empty() && (value == a || value == b) &&
                 !to.empty() && to == instruction(hold1->before);
    }
    const bool suffix = shared(target->after, press0->after, press1->after, hold0->after, hold1->after);
    if (!prefix && !suffix)
        return std::string(original);
    std::string result(prefix ? hold0->before : target->before);
    result += target->button;
    result += suffix ? hold0->after : target->after;
    return result;
}
} // namespace mw3gf
