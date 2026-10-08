#include "mw3gf/core/gameplay_prompts.hpp"
#include "mw3gf/core/gameplay_adapter.hpp"
#include "mw3gf/core/prompt_catalog.hpp"
#include <algorithm>
#include <limits>
namespace mw3gf {
namespace {
bool has_button(std::string_view text, unsigned code) {
    for (std::size_t at = 0; at < text.size(); ++at) {
        if (const auto bytes = inline_material_bytes(text, at)) {
            at += bytes - 1;
            continue;
        }
        if (static_cast<unsigned char>(text[at]) == code)
            return true;
    }
    return false;
}
std::string fixed_prompt(std::string text, const detail::FixedPrompt& prompt) {
    if (prompt.kind == detail::FixedPromptKind::icon_only)
        return std::string(prompt.glyphs);
    if (std::all_of(prompt.glyphs.begin(), prompt.glyphs.end(),
                    [&](unsigned char code) { return code == ' ' || has_button(text, code); }))
        return text;
    if (prompt.kind == detail::FixedPromptKind::parameter) {
        std::size_t at{};
        while ((at = text.find("&&1", at)) != std::string::npos) {
            text.replace(at, 3, prompt.glyphs);
            at += prompt.glyphs.size();
        }
        return contains_prompt_glyph(text) ? text : std::string(prompt.glyphs) + " " + text;
    }
    if (prompt.kind == detail::FixedPromptKind::label)
        return contains_prompt_glyph(text) ? text : std::string(prompt.glyphs) + " " + text;
    // Only these audited contexts contain a keyboard/mouse input field.
    // Replace marked fields, never translated words or arbitrary label text.
    bool replaced{};
    for (std::size_t at = 0; at < text.size();) {
        if (const auto bytes = inline_material_bytes(text, at)) {
            at += bytes;
            continue;
        }
        const bool colored =
            text[at] == '^' && at + 1 < text.size() && (text[at + 1] == '2' || text[at + 1] == '3');
        const bool bracketed = text[at] == '[';
        if (!colored && !bracketed) {
            ++at;
            continue;
        }
        const auto begin = at + (colored ? 2 : 1);
        auto end = colored ? text.find('^', begin) : text.find(']', begin);
        if (end == std::string::npos)
            end = text.size();
        const std::string_view field(text.data() + begin, end - begin);
        if (contains_prompt_glyph(field) || field.find("&&") != std::string_view::npos ||
            field.find('{') != std::string_view::npos) {
            at = end;
            continue;
        }
        const auto count = end - begin + (!colored && end < text.size() ? 1 : 0);
        const std::string replacement =
            std::string(prompt.glyphs) + (!field.empty() && field.back() == ' ' ? " " : "");
        text.replace(colored ? begin : at, colored ? count : count + 1, replacement);
        at = (colored ? begin : at) + replacement.size();
        replaced = true;
        break;
    }
    if (replaced || contains_prompt_glyph(text) || text.find("&&") != std::string::npos)
        return text;
    return std::string(prompt.glyphs) + " " + text;
}
} // namespace
std::optional<unsigned> action_prompt_glyph(std::string_view action) noexcept {
    return gameplay_glyph(action);
}
std::string expand_action_prompts(std::string_view text) {
    std::string result(text);
    std::size_t at{};
    while (at < result.size()) {
        if (const auto bytes = inline_material_bytes(result, at)) {
            at += bytes;
            continue;
        }
        if (result[at] != '{') {
            ++at;
            continue;
        }
        const auto end = result.find('}', at + 1);
        if (end == std::string::npos)
            break;
        const auto glyph = action_prompt_glyph(std::string_view(result).substr(at + 1, end - at - 1));
        if (!glyph) {
            at = end + 1;
            continue;
        }
        const bool brackets =
            at > 0 && result[at - 1] == '[' && end + 1 < result.size() && result[end + 1] == ']';
        const auto begin = brackets ? at - 1 : at;
        result.replace(begin, end - at + 1 + (brackets ? 2 : 0), 1, static_cast<char>(*glyph));
        at = begin + 1;
    }
    for (std::size_t i = 0; i + 2 < result.size(); ++i) {
        if (const auto bytes = inline_material_bytes(result, i)) {
            i += bytes - 1;
            continue;
        }
        if (result[i] == '[' && is_prompt_glyph(static_cast<unsigned char>(result[i + 1])) &&
            result[i + 2] == ']') {
            result.erase(i + 2, 1);
            result.erase(i, 1);
        }
    }
    return result;
}
bool contains_prompt_glyph(std::string_view text) noexcept {
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (const auto bytes = inline_material_bytes(text, i)) {
            i += bytes - 1;
            continue;
        }
        if (is_prompt_glyph(static_cast<unsigned char>(text[i])))
            return true;
    }
    return false;
}
std::string controller_instruction(std::string_view key, std::string_view original) {
    if (key.starts_with('@'))
        key.remove_prefix(1);
    if (key == "KEY_USE")
        return std::string(1, icon_character(ButtonIcon::x));
    if (key == "KEY_ESCAPE" || key == "PLATFORM_BACK_BUTTON")
        return std::string(1, icon_character(ButtonIcon::b));
    if (key == "KEY_ENTER")
        return std::string(1, icon_character(ButtonIcon::a));
    // These PC resources have no action slot. Use a context-only icon rather
    // than guessing where a translated device name begins/ends. Keep the
    // existing PC input mappings; Xbox remote context bindings differ.
    if (key == "MENU_PRESS_START" || key == "MENU_PRESS_START_TO_SKIP")
        return std::string(1, icon_character(ButtonIcon::start));
    if (key == "PLATFORM_USE_BUTTONLOOK_TO_AIM")
        return std::string(1, icon_character(ButtonIcon::left_trigger));
    if (key == "PLATFORM_PRESS_BUTTON_TO_CONFIRM_TARGET")
        return std::string(1, icon_character(ButtonIcon::right_trigger));
    if (key == "PLATFORM_HINT_MOVEONTRUCK")
        return std::string(1, icon_character(ButtonIcon::left_stick));
    if (key == "PLATFORM_LOCSEL_POSITION_CONTROLS" || key == "PLATFORM_USE_BUTTONMOVE_TO_POSITION")
        return std::string(1, icon_character(ButtonIcon::right_stick));
    if (key == "BERLIN_DIRECT_A10_POINTS") {
        auto text = expand_action_prompts(original);
        const char stick = icon_character(ButtonIcon::right_stick);
        return text.find(stick) == std::string::npos ? std::string(1, stick) + " " + text : text;
    }
    std::string result = expand_action_prompts(original);
    if (key == "PLATFORM_FRIENDS_BUTTON")
        return std::string(1, icon_character(ButtonIcon::y));
    if (key == "PLATFORM_SUMMARY_BUTTON")
        return std::string(1, icon_character(ButtonIcon::back));
    if (key == "PLATFORM_CHALLENGE_BLADE_BUTTON")
        return std::string(1, icon_character(ButtonIcon::y));
    const auto rule = std::lower_bound(
        detail::fixed_prompts.begin(), detail::fixed_prompts.end(), key,
        [](const detail::FixedPrompt& prompt, std::string_view value) { return prompt.key < value; });
    if (rule != detail::fixed_prompts.end() && rule->key == key)
        return fixed_prompt(std::move(result), *rule);
    return result;
}
std::size_t inline_material_bytes(std::string_view text, std::size_t at) noexcept {
    if (at >= text.size() || text[at] != '^' || text.size() - at < 2)
        return 0;
    const auto mode = static_cast<unsigned char>(text[at + 1]);
    if (mode != 1 && mode != 2)
        return 0;
    // A truncated record is opaque too: don't interpret binary fields as keys.
    if (text.size() - at < 5)
        return text.size() - at;
    const auto length = static_cast<unsigned char>(text[at + 4]);
    return std::min(text.size() - at, std::size_t{5} + length);
}
unsigned image_dimension_byte(unsigned pixels) noexcept {
    return std::clamp(16u + (pixels * 32u + 16u) / 33u, 17u, 127u);
}
int image_dimension(unsigned encoded, int font_height) noexcept {
    return ((static_cast<int>(encoded) - 16) * font_height + 16) / 32;
}
std::string inline_button_record(unsigned code, unsigned width, unsigned height) {
    if (!is_prompt_glyph(code))
        return {};
    const std::string name =
        "mw3gf/p" + std::string(1, static_cast<char>('0' + code / 10)) + static_cast<char>('0' + code % 10);
    std::string result("^\x01", 2);
    result += static_cast<char>(image_dimension_byte(width));
    result += static_cast<char>(image_dimension_byte(height));
    result += static_cast<char>(name.size());
    result += name;
    return result;
}
std::optional<int> prompt_text_width(std::string_view text, int max_chars, int font_height,
                                     PromptReadCharacter read, PromptCharacterAdvance advance,
                                     void* context) {
    if (!read || !advance || font_height <= 0 || font_height > 128)
        return std::nullopt;
    const char* position = text.data();
    const char* end = position + text.size();
    int line{}, maximum{}, visible{};
    const int limit = max_chars > 0 ? max_chars : std::numeric_limits<int>::max();
    while (position < end && visible < limit) {
        const auto start = position;
        const auto offset = static_cast<std::size_t>(start - text.data());
        if (const auto bytes = inline_material_bytes(text, offset)) {
            if (bytes < 5)
                return std::nullopt;
            const auto width = static_cast<unsigned char>(start[2]);
            const auto height = static_cast<unsigned char>(start[3]);
            const auto length = static_cast<unsigned char>(start[4]);
            if (width >= 128 || height >= 128 || length >= 128 || bytes != 5u + length)
                return std::nullopt;
            line += image_dimension(width, font_height);
            position += bytes;
            ++visible;
        } else {
            const unsigned code = read(&position, context);
            if (position <= start || position > end)
                return std::nullopt;
            if (code == '\n' || code == '\r') {
                line = 0;
                continue;
            }
            if (code == '^' && position < end && *position >= '0' && *position <= ';') {
                ++position;
                continue;
            }
            line += advance(code, context);
            ++visible;
        }
        maximum = std::max(maximum, line);
    }
    return maximum;
}
std::optional<int> prompt_visible_characters(std::string_view text, PromptReadCharacter read, void* context) {
    if (!read)
        return std::nullopt;
    const char* position = text.data();
    const char* end = position + text.size();
    int count{};
    while (position < end) {
        const auto start = position;
        if (const auto bytes = inline_material_bytes(text, static_cast<std::size_t>(start - text.data()))) {
            if (bytes < 5 || bytes != 5u + static_cast<unsigned char>(start[4]) ||
                static_cast<unsigned char>(start[4]) >= 128)
                return std::nullopt;
            position += bytes;
            ++count;
        } else {
            const auto code = read(&position, context);
            if (position <= start || position > end)
                return std::nullopt;
            if (code == '\n' || code == '\r')
                continue;
            if (code == '^' && position < end && *position >= '0' && *position <= ';') {
                ++position;
                continue;
            }
            ++count;
        }
    }
    return count;
}
} // namespace mw3gf
