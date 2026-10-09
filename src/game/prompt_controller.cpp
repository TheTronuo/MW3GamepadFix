#include "mw3gf/game/prompt_controller.hpp"
#include "mw3gf/core/gameplay_prompts.hpp"
#include "mw3gf/game/native_layout.hpp"
#include "mw3gf/platform/windows.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace mw3gf::game {
bool PromptController::controller_prompts() const noexcept {
    return controller_mode_.load(std::memory_order_acquire) && context_.settings.gameplay_prompts;
}
void* PromptController::prompt_material(const char* name) noexcept {
    try {
        return name ? icons_.material(safe_string(name)) : nullptr;
    } catch (...) {
        return nullptr;
    }
}
int PromptController::measured_width(const char* text, int max_chars, NativeFont* font) noexcept {
    if (const auto width = measured_prompt_width(text, max_chars, font, 0))
        return *width;
    return context_.engine.originals().text_width(text, max_chars, font);
}
int PromptController::measured_width(const char* text, int max_chars, NativeFont* font,
                                     int decode_mode) noexcept {
    if (const auto width = measured_prompt_width(text, max_chars, font, decode_mode))
        return *width;
    return context_.engine.originals().decoded_text_width(text, max_chars, font, decode_mode);
}
std::optional<int> PromptController::measured_prompt_width(const char* text, int max_chars, NativeFont* font,
                                                           int decode_mode) noexcept {
    try {
        if (text && controller_prompts() && readable(font, sizeof(NativeFont))) {
            const auto expanded = prompt_text(safe_string(text));
            if (contains_prompt_glyph(expanded) || expanded.find("mw3gf/p") != std::string::npos) {
                struct Context {
                    PromptController* bridge;
                    NativeFont* font;
                    int decode_mode;
                } context{this, font, decode_mode};
                const auto read = [](const char** position, void* opaque) -> unsigned {
                    const auto& state = *static_cast<Context*>(opaque);
                    return state.bridge->context_.engine.function<ReadCharacter>(read_character_rva)(
                        position, 0, state.decode_mode);
                };
                const auto advance = [](unsigned code, void* opaque) -> int {
                    const auto& state = *static_cast<Context*>(opaque);
                    return state.bridge->glyph(state.font, code)->advance;
                };
                if (const auto width =
                        prompt_text_width(expanded, max_chars, font->pixel_height, read, advance, &context))
                    return *width;
            }
        }
    } catch (...) {
    }
    return std::nullopt;
}
int PromptController::visible_characters(const char* text) noexcept {
    try {
        if (text && icons_.ready()) {
            const auto value = safe_string(text, 0x10000);
            if (value.find("mw3gf/p") != std::string::npos) {
                const auto read = [](const char** position, void* opaque) -> unsigned {
                    const auto& state = *static_cast<PromptController*>(opaque);
                    return state.context_.engine.function<ReadCharacter>(read_character_rva)(position, 0, 0);
                };
                if (const auto count = prompt_visible_characters(value, read, this))
                    return *count;
            }
        }
    } catch (...) {
    }
    return context_.engine.originals().text_count(text);
}
void PromptController::render_text(void* state) noexcept {
    try {
        if (render_button_text(state))
            return;
    } catch (...) { /* Fall back to the original queued command. */
    }
    context_.engine.originals().render_text(state);
}
bool PromptController::render_button_text(void* state) {
    if (!icons_.ready() || !readable(state, sizeof(void*)))
        return false;
    auto* command = *static_cast<std::byte**>(state);
    if (!readable(command, layout::text_command::text + 1))
        return false;
    const auto size = read_field<std::uint16_t>(command, layout::text_command::size);
    if (size < layout::text_command::minimum_size || !readable(command, size))
        return false;
    const char* text = reinterpret_cast<const char*>(command + layout::text_command::text);
    const auto* end = static_cast<const char*>(std::memchr(text, 0, size - layout::text_command::text));
    if (!end)
        return false;
    const auto semantic =
        controller_prompts() ? prompt_text(std::string_view(text, end - text)) : std::string(text, end);
    if (!contains_prompt_glyph(semantic))
        return false;
    const auto expanded = icons_.inline_text(semantic);
    if (expanded.size() > layout::text_command::maximum_size - layout::text_command::minimum_size)
        return false;

    // Preserve the engine's font, header and queued source bytes. Render using
    // a private aligned command, then advance the real iterator by its old size.
    constexpr std::size_t alignment = sizeof(std::uint64_t);
    const auto bytes = layout::text_command::text + expanded.size() + 1;
    std::vector<std::uint64_t> storage((bytes + alignment - 1) / alignment);
    auto* copy = reinterpret_cast<std::byte*>(storage.data());
    std::memcpy(copy, command, layout::text_command::text);
    std::memcpy(copy + layout::text_command::text, expanded.c_str(), expanded.size() + 1);
    void* iterator = copy;
    context_.engine.originals().render_text(&iterator);
    *static_cast<std::byte**>(state) = command + size;
    return true;
}
void PromptController::renderer_release() noexcept {
    controller_mode_.store(false, std::memory_order_release);
    // Native callers have synchronized the backend before releasing D3D
    // resources. This also runs for vid_restart and remains hooked after Stop.
    icons_.release_gpu_resources();
    images_verified_ = false;
    prompt_failed_ = false;
    context_.logger.write("Native renderer teardown: mod textures released before device shutdown/reset.");
}
int PromptController::binding_keys(int client, const char* command, char* output) noexcept {
    if (client != 0 || !controller_prompts() || !command || !output)
        return -1;
    try {
        const auto glyph = action_prompt_glyph(safe_string(command));
        if (!glyph)
            return -1;
        output[0] = static_cast<char>(*glyph);
        output[1] = '\0';
        output[128] = '\0';
        return 1;
    } catch (...) {
        return -1;
    }
}
std::string PromptController::instruction(std::string_view key, std::string_view text) {
    std::string converted(text);
    if (context_.settings.gameplay) {
        if (is_mortar_prompt(key)) {
            const auto translate = context_.engine.originals().localized_text;
            std::array<std::string, 2> press, hold;
            for (std::size_t i = 0; i < press.size(); ++i) {
                press[i] = safe_string(translate(mortar_press_reference_keys[i].data()));
                hold[i] = safe_string(translate(mortar_hold_reference_keys[i].data()));
            }
            converted = localized_mortar_instruction(key, text, {{press[0], press[1]}, {hold[0], hold[1]}});
        }
        if (is_prone_prompt(key)) {
            const auto hold = safe_string(
                context_.engine.originals().localized_text(prone_hold_reference_key.data()));
            converted = localized_prone_instruction(key, text, hold);
        }
        if (const auto* rule = find_use_hold_prompt(key)) {
            const auto translate = context_.engine.originals().localized_text;
            std::array<std::string, 2> press, hold;
            for (std::size_t i = 0; i < press.size(); ++i) {
                press[i] = safe_string(translate(rule->press_reference_keys[i].data()));
                hold[i] = safe_string(translate(use_hold_reference_keys[i].data()));
            }
            converted = localized_hold_instruction(
                text, {{press[0], press[1]}, {hold[0], hold[1]}, rule->allow_implicit_prefix});
            if (rule->action == UseAction::ground_weapon || rule->action == UseAction::throwing_knife) {
                std::array<std::string, 2> compact_hold;
                for (std::size_t i = 0; i < compact_hold.size(); ++i)
                    compact_hold[i] = safe_string(translate(weapon_hold_reference_keys[i].data()));
                if (const auto pickup = localized_weapon_hold_instruction(
                        key, text, {{press[0], press[1]}, {compact_hold[0], compact_hold[1]},
                                    rule->allow_implicit_prefix}))
                    converted = *pickup;
            }
        }
    }
    return controller_instruction(key, converted);
}
const char* PromptController::localized(const char* key, const char* original) noexcept {
    if (!key || !original)
        return original;
    try {
        const auto name = safe_string(key), text = safe_string(original);
        if (name.empty() || text.empty())
            return original;
        const auto replacement = instruction(name, text);
        if (replacement == text)
            return original;
        std::scoped_lock lock(prompt_strings_mutex_);
        const auto found = prompt_strings_.emplace(std::pair{name, text}, replacement);
        // Track actual localized values even before a pad is connected.
        // Cached copies can then be recognized without any RU/EN word list.
        // Plain labels can be shared by unrelated resources. Only cache
        // transformed instructions, never a label with an added button.
        if (text.size() > 8 && replacement.find(text) == std::string::npos)
            cached_instructions_.emplace(text, replacement);
        return controller_prompts() ? found.first->second.c_str() : original;
    } catch (...) {
        return original;
    }
}
void* PromptController::hud_localized_asset(int type, const char* key, void* original,
                                           std::uintptr_t caller_rva) noexcept {
    if (!controller_prompts() || type != localize_asset_type || caller_rva != hud_localize_return_rva ||
        !key || !readable(original, sizeof(NativeLocalizeEntry)))
        return original;
    try {
        const auto name = safe_string(key);
        if (!is_hud_prompt_lookup(type, name, caller_rva) ||
            (name != sdv_prompt_key && !context_.settings.gameplay))
            return original;
        NativeLocalizeEntry entry{};
        std::memcpy(&entry, original, sizeof(entry));
        const auto text = safe_string(entry.value);
        return const_cast<NativeLocalizeEntry*>(hud_prompts_.lookup(
            static_cast<NativeLocalizeEntry*>(original), name, text, instruction(name, text)));
    } catch (...) {
        return original;
    }
}
Glyph* PromptController::glyph(NativeFont* font, unsigned code) noexcept {
    if (controller_prompts() && is_prompt_glyph(code)) {
        try {
            if (auto* glyph = icons_.button_glyph(font, code))
                return glyph;
        } catch (...) {
        }
    }
    return context_.engine.originals().lookup_glyph(font, code);
}
void* PromptController::text_command(const char* text, int max_chars, NativeFont* font, float x, float y,
                                     float xs, float ys, float rotation, const float* color, int style,
                                     int cursor, bool flag) noexcept {
    try {
        if (text && controller_prompts()) {
            const auto expanded = prompt_text(safe_string(text));
            if (contains_prompt_glyph(expanded)) {
                return context_.engine.originals().text_command(expanded.c_str(), max_chars, font, x, y, xs,
                                                                ys, rotation, color, style, cursor, flag);
            }
        }
    } catch (...) {
    }
    return context_.engine.originals().text_command(text, max_chars, font, x, y, xs, ys, rotation, color,
                                                    style, cursor, flag);
}
std::string PromptController::prompt_text(std::string_view text) {
    {
        std::scoped_lock lock(prompt_strings_mutex_);
        if (const auto found = cached_instructions_.find(std::string(text));
            found != cached_instructions_.end())
            return found->second;
    }
    return expand_action_prompts(text);
}
void PromptController::prepare(NativeFont* font, bool connected) noexcept {
    if (!context_.settings.gameplay_prompts || !readable(font, sizeof(NativeFont)))
        return;
    try {
        if (connected && !prompt_failed_ && !icons_.ready()) {
            (void)icons_.prepare(font, context_.engine.read<void*>(cursor_material_pointer_rva));
        }
        if (!images_verified_ && icons_.ready()) {
            images_verified_ = true;
            context_.logger.write(
                "Independent button images ready: 16 UI materials; original localized fonts untouched.");
        }
    } catch (const std::exception& error) {

        prompt_failed_ = true;
        context_.logger.write(std::string("Prompt preparation failed; gameplay input remains active: ") +
                              error.what());
    } catch (...) {
        prompt_failed_ = true;
    }
}
} // namespace mw3gf::game

namespace mw3gf::game {
void PromptController::update_mode(bool connected) noexcept {
    controller_mode_.store(connected && foreground() && images_verified_ && icons_.font() &&
                               !context_.engine.read<int>(binding_capture_rva) &&
                               !context_.engine.read<int>(edit_capture_rva),
                           std::memory_order_release);
}
} // namespace mw3gf::game
