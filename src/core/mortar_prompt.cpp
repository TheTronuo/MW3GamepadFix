#include "mw3gf/core/mortar_prompt.hpp"
namespace mw3gf {
bool is_mortar_prompt(std::string_view key) noexcept {
    return use_hint_key(key) == mortar_prompt_key;
}
std::string localized_mortar_instruction(std::string_view key, std::string_view original,
                                         const HoldTextTemplates& templates) {
    if (!is_mortar_prompt(key))
        return std::string(original);
    return localized_hold_instruction(original, {templates.hold, templates.press});
}
} // namespace mw3gf
