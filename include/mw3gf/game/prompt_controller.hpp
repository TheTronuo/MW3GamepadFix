#pragma once
#include "mw3gf/core/gameplay_adapter.hpp"
#include "mw3gf/game/button_icons.hpp"
#include "mw3gf/game/hud_prompt.hpp"
#include "mw3gf/game/runtime_context.hpp"
#include <atomic>
#include <map>
#include <optional>
#include <vector>
namespace mw3gf::game {
class PromptController {
  public:
    PromptController(RuntimeContext& context, HMODULE module)
        : context_(context), icons_(module, context.settings.prompt_style) {}
    ButtonIcons& icons() noexcept { return icons_; }
    bool failed() const noexcept { return prompt_failed_; }
    void mark_failed() noexcept { prompt_failed_ = true; }
    void update_mode(bool connected) noexcept;
    void prepare(NativeFont* font, bool connected) noexcept;
    bool controller_prompts() const noexcept;
    void* prompt_material(const char* name) noexcept;
    int measured_width(const char* text, int max_chars, NativeFont* font) noexcept;
    int measured_width(const char* text, int max_chars, NativeFont* font, int decode_mode) noexcept;
    int visible_characters(const char* text) noexcept;
    void render_text(void* state) noexcept;
    void renderer_release() noexcept;
    int binding_keys(int client, const char* command, char* output) noexcept;
    const char* localized(const char* key, const char* original) noexcept;
    void* hud_localized_asset(int type, const char* key, void* original,
                              std::uintptr_t caller_rva) noexcept;
    Glyph* glyph(NativeFont* font, unsigned code) noexcept;
    void* text_command(const char* text, int max_chars, NativeFont* font, float x, float y, float xs,
                       float ys, float rotation, const float* color, int style, int cursor,
                       bool flag) noexcept;

  private:
    std::optional<int> measured_prompt_width(const char* text, int max_chars, NativeFont* font,
                                             int decode_mode) noexcept;
    bool render_button_text(void* state);
    std::string prompt_text(std::string_view text);
    RuntimeContext& context_;
    std::atomic_bool controller_mode_{};
    std::mutex prompt_strings_mutex_;
    std::map<std::pair<std::string, std::string>, std::string> prompt_strings_;
    std::map<std::string, std::string> cached_instructions_;
    HudPromptCache hud_prompts_;
    bool images_verified_{}, prompt_failed_{};
    ButtonIcons icons_;
};
} // namespace mw3gf::game
