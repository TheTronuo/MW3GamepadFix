#pragma once
#include "mw3gf/core/menu_footer.hpp"
#include "mw3gf/game/button_icons.hpp"
#include "mw3gf/game/menu_actions.hpp"
#include "mw3gf/game/prompt_controller.hpp"
#include "mw3gf/game/runtime_context.hpp"
#include <vector>
namespace mw3gf::game {
class MenuController {
    struct Footer {
        std::array<void*, 4> items{};
        FooterBinding binding{};
        std::string label;
        int engine_key{};
    };

  public:
    MenuController(RuntimeContext& context, PromptController& prompts)
        : context_(context), prompts_(prompts) {}
    bool hide_cursor(void* material) const noexcept;
    void pump_input() noexcept;
    void begin_footer_frame(int local_client) noexcept;
    [[nodiscard]] bool replaced_footer(void* item) const noexcept;
    void paint(int local_client) noexcept;

  private:
    [[nodiscard]] NativeMenuActions native_actions() const;
    static std::string footer_key(void* item);
    bool footer_enabled(const Footer& footer) const;
    bool dispatch_footer(void* menu, MenuAction action);
    void* current_menu() const;
    static int focused_index(void* menu);
    static void* focused_item(void* menu);
    static bool has_key_handler(void* menu, int key);
    void ensure_focus(void* menu);
    float text_width(std::string_view text, NativeFont* font, float scale) const;
    bool eligible(void* item) const;
    void draw_icon(const char* glyphs, NativeFont* font, float x, float y, float scale,
                   const float* color = nullptr) const;
    bool draw_row_prompt(void* item, NativeFont* font, float viewport_width, float prompt_height);
    void draw_prompts(void* menu, NativeFont* font, float x, float y, float scale, float height,
                      float viewport_width);
    void prompt(const char* glyphs, std::wstring_view fallback, std::string_view label, NativeFont* normal,
                float& right, float y, float scale, float height, bool available);
    void draw(std::wstring_view text, void* font, float x, float y, float scale);
    RuntimeContext& context_;
    PromptController& prompts_;
    void* focused_menu_{};
    void* footer_menu_{};
    std::vector<Footer> footers_;
    std::string footer_signature_;
    std::wstring last_text_;
    bool menu_frame_seen_{}, first_frame_{}, input_error_logged_{}, error_logged_{};
};
} // namespace mw3gf::game
