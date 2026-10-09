#include "mw3gf/core/prone_prompt.hpp"
#include "mw3gf/core/button_icon.hpp"
#include "mw3gf/core/gameplay_prompts.hpp"
#include "mw3gf/core/use_policy.hpp"
#include <algorithm>
namespace mw3gf {
namespace {
bool single_button_parameter(std::string_view text) noexcept {
    const auto at = text.find("&&");
    return at != std::string_view::npos && text.substr(at, 3) == "&&1" &&
           text.find("&&", at + 3) == std::string_view::npos &&
           (at + 3 == text.size() || text[at + 3] < '0' || text[at + 3] > '9');
}
} // namespace
bool is_prone_prompt(std::string_view key) noexcept {
    key = use_hint_key(key);
    return std::find(prone_prompt_keys.begin(), prone_prompt_keys.end(), key) != prone_prompt_keys.end();
}
std::string localized_prone_instruction(std::string_view key, std::string_view original,
                                        std::string_view hold) {
    if (!is_prone_prompt(key) || original.empty() || !single_button_parameter(hold) ||
        hold.find_first_of("{}") != std::string_view::npos || contains_prompt_glyph(hold) ||
        (original.find("&&") != std::string_view::npos && !single_button_parameter(original)))
        return std::string(original);
    std::string result(hold);
    if (original.find("&&1") == std::string_view::npos)
        result.replace(result.find("&&1"), 3, 1, icon_character(ButtonIcon::b));
    return result;
}
} // namespace mw3gf
