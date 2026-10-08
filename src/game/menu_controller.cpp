#include "mw3gf/game/menu_controller.hpp"
#include "mw3gf/core/gameplay_prompts.hpp"
#include "mw3gf/core/prompt_layout.hpp"
#include "mw3gf/core/status_text.hpp"
#include "mw3gf/game/native_layout.hpp"
#include "mw3gf/platform/windows.hpp"
#include <algorithm>
#include <cmath>
namespace mw3gf::game {
bool MenuController::hide_cursor(void* material) const noexcept {
    return context_.settings.navigation && foreground() && context_.input.monitor().snapshot().connected &&
           material && material == context_.engine.read<void*>(cursor_material_pointer_rva);
}
void MenuController::begin_footer_frame(int local_client) noexcept {
    footers_.clear();
    footer_menu_ = nullptr;
    try {
        if (local_client != 0 || !context_.settings.prompts ||
            !context_.input.monitor().snapshot().connected || !prompts_.icons().font())
            return;
        DWORD process{};
        GetWindowThreadProcessId(GetForegroundWindow(), &process);
        if (process != GetCurrentProcessId())
            return;
        footer_menu_ = current_menu();
        if (!footer_menu_)
            return;
        const int count = read_field<int>(footer_menu_, layout::menu::item_count);
        const auto* items = read_field<void*>(footer_menu_, layout::menu::items);
        if (!items || count < 1 || count > 1024)
            return;
        for (int i = 0; i < count; ++i) {
            void* label_item = read_field<void*>(items, static_cast<std::size_t>(i) * 8);
            const auto key = footer_key(label_item);
            const auto binding = footer_binding(key);
            if (!binding)
                continue;
            Footer footer;
            footer.binding = *binding;
            bool group = i >= 3;
            for (int n = 0; group && n < 4; ++n) {
                footer.items[static_cast<std::size_t>(n)] =
                    read_field<void*>(items, static_cast<std::size_t>(i - 3 + n) * 8);
                const void* part = footer.items[static_cast<std::size_t>(n)];
                group &= read_field<std::uint8_t>(part, layout::item::horizontal_alignment) == 3 &&
                         read_field<std::uint8_t>(part, layout::item::vertical_alignment) == 3;
            }
            // The shipped mouse footer consists of a button, two hover
            // decorations and its label. Never suppress unrelated text.
            if (read_field<int>(label_item, layout::item::type) == 1 &&
                read_field<void*>(label_item, layout::item::action))
                footer.items = {label_item, nullptr, nullptr, nullptr}; // Older direct-text footer buttons.
            else if (!group || read_field<int>(footer.items[0], layout::item::type) != 1 ||
                     !read_field<void*>(footer.items[0], layout::item::action)) {
                // Spec Ops Leaderboards is a text-only existing shortcut
                // (localized Right Mouse/F1), not a four-item mouse button.
                if (binding->action != MenuAction::auxiliary_x ||
                    read_field<int>(label_item, layout::item::type) != 0)
                    continue;
                footer.items = {label_item, nullptr, nullptr, nullptr};
                footer.engine_key = 167; // PC F1, from PLATFORM_LEADERBOARDS_SHORTCUT.
            }
            const auto visible = context_.engine.function<int (*)(int, void*)>(item_visible_rva);
            if (!visible(0, label_item) || !visible(0, footer.items[0]))
                continue;
            const auto localized =
                context_.engine.function<const char* (*)(const char*)>(localize_rva)(key.c_str() + 1);
            footer.label = footer_label(safe_string(localized));
            if (!footer.label.empty() &&
                std::none_of(footers_.begin(), footers_.end(), [&](const Footer& existing) {
                    return existing.binding.action == footer.binding.action;
                }))
                footers_.push_back(std::move(footer));
        }
        std::string signature;
        for (const auto& footer : footers_)
            signature += "[" + footer.label + "]";
        if (signature != footer_signature_) {
            context_.logger.write("PC footer replaced: " + signature);
            footer_signature_ = std::move(signature);
        }
    } catch (const std::exception& error) {
        footers_.clear();
        context_.logger.write(error.what());
    } catch (...) {
        footers_.clear();
    }
}
bool MenuController::replaced_footer(void* item) const noexcept {
    // A disconnect or focus loss restores the original UI immediately.
    if (!item || !context_.input.monitor().snapshot().connected || current_menu() != footer_menu_)
        return false;
    DWORD process{};
    GetWindowThreadProcessId(GetForegroundWindow(), &process);
    if (process != GetCurrentProcessId())
        return false;
    for (const auto& footer : footers_)
        if (std::find(footer.items.begin(), footer.items.end(), item) != footer.items.end())
            return true;
    // Some Spec Ops menus have an additional noninteractive copy of the
    // Leaderboards shortcut label, outside the four-item mouse group.
    try {
        if (read_field<void*>(item, layout::item::parent) == footer_menu_) {
            const auto binding = footer_binding(footer_key(item));
            if (binding)
                for (const auto& footer : footers_)
                    if (footer.binding.action == binding->action)
                        return true;
        }
    } catch (...) {
        return false;
    }
    return false;
}
void MenuController::pump_input() noexcept {
    try {
        const HWND window = GetForegroundWindow();
        DWORD process{};
        GetWindowThreadProcessId(window, &process);
        context_.input.poll_menu();
        const auto& current = context_.input.monitor().snapshot();
        const std::wstring text =
            (context_.demo ? L"[DEMO] " : L"") + context_.settings.label + L": " + pressed_controls(current);
        if (text != last_text_) {
            context_.logger.write(win::utf8(text) + " | " + win::utf8(current.backend));
            last_text_ = text;
        }
        // Refresh shortcut actions for this menu before dispatch; the next
        // paint refreshes them again if a callback opens another menu.
        begin_footer_frame(0);
        void* menu = current_menu();
        const bool active = process == GetCurrentProcessId() && menu != nullptr;
        if (active && current.connected && context_.settings.navigation && !context_.demo) {
            if (menu != focused_menu_ || replaced_footer(focused_item(menu))) {
                ensure_focus(menu);
                focused_menu_ = menu;
                menu = current_menu();
            }
        } else
            focused_menu_ = nullptr;
        const auto identity = reinterpret_cast<std::uintptr_t>(menu);
        std::uint32_t shortcuts{};
        for (const auto& footer : footers_) {
            if (footer_menu_ != menu || !footer_enabled(footer))
                continue;
            if (footer.binding.action == MenuAction::auxiliary_y)
                shortcuts |= bit(Button::y);
            if (footer.binding.action == MenuAction::auxiliary_x)
                shortcuts |= bit(Button::x);
            if (footer.binding.action == MenuAction::auxiliary_view)
                shortcuts |= bit(Button::back);
            if (footer.binding.action == MenuAction::page_up)
                shortcuts |= bit(Button::lb);
            if (footer.binding.action == MenuAction::page_down)
                shortcuts |= bit(Button::rb);
            if (footer.binding.action == MenuAction::top)
                shortcuts |= bit(Button::ls);
            if (footer.binding.action == MenuAction::bottom)
                shortcuts |= bit(Button::rs);
        }
        const auto commands = context_.menu_adapter.update(
            current, {active && menu && context_.settings.navigation && !context_.demo, identity, shortcuts},
            GetTickCount64());
        const auto dispatch = native_actions();
        constexpr std::array keys{13, 27, 154, 155, 156, 157};
        for (std::size_t i = 0; i < commands.count; ++i) {
            // A callback can open/close a menu. Never apply the remaining
            // commands from this snapshot to a different input consumer.
            if (current_menu() != menu)
                break;
            const auto action = commands.values[i];
            if (context_.engine.read<int>(binding_capture_rva) && action != MenuAction::back)
                continue;
            if (!context_.engine.read<int>(binding_capture_rva) &&
                !context_.engine.read<int>(edit_capture_rva) && dispatch_footer(menu, action))
                continue;
            if (static_cast<std::size_t>(action) >= keys.size())
                continue;
            const int key = keys[static_cast<std::size_t>(action)];
            const int before = focused_index(menu);
            dispatch.press_key(key);
            context_.logger.write("UI key=" + std::to_string(key) + " focus=" + std::to_string(before) +
                                  " -> " + std::to_string(current_menu() == menu ? focused_index(menu) : -1) +
                                  " menu_changed=" + std::to_string(current_menu() != menu));
        }
        input_error_logged_ = false;
    } catch (const std::exception& error) {
        if (!input_error_logged_) {
            context_.logger.write(error.what());
            input_error_logged_ = true;
        }
    } catch (...) {
        if (!input_error_logged_) {
            context_.logger.write("Unknown menu input exception");
            input_error_logged_ = true;
        }
    }
}
void MenuController::paint(int local_client) noexcept {
    try {
        if (local_client != 0)
            return;
        const int count = context_.engine.read<int>(menu_context_rva + open_menu_count_offset);
        if (!menu_frame_seen_) {
            context_.logger.write("First menu frame observed; open menu stack=" + std::to_string(count));
            menu_frame_seen_ = true;
        }
        const HWND window = GetForegroundWindow();
        DWORD process{};
        GetWindowThreadProcessId(window, &process);
        const auto& current = context_.input.monitor().snapshot();
        void* menu = current_menu();
        const bool active = process == GetCurrentProcessId() && menu != nullptr;
        if (!active || !menu || !current.connected || count <= 0 || count > menu_stack_capacity)
            return;
        RECT client{};
        if (!GetClientRect(window, &client) || client.right <= 0 || client.bottom <= 0)
            return;
        auto* font = context_.engine.read<NativeFont*>(normal_font_pointer_rva);
        if (!readable(font, sizeof(NativeFont)))
            return;
        const int native_height = font->pixel_height;
        if (native_height <= 0 || native_height > 256)
            return;
        const float height = std::clamp(static_cast<float>(client.bottom) *
                                            static_cast<float>(context_.settings.height_percent) / 100.0f,
                                        16.0f, 96.0f);
        const float scale = height / static_cast<float>(native_height);
        const float x =
            static_cast<float>(client.right) * static_cast<float>(context_.settings.x_percent) / 100.0f;
        const float y =
            static_cast<float>(client.bottom) * static_cast<float>(context_.settings.y_percent) / 100.0f;
        if (context_.settings.prompts)
            draw_prompts(menu, font, x, y, scale, height, static_cast<float>(client.right));
        if (context_.settings.show_debug)
            draw(last_text_, font, x, y - height * 1.35f, scale);
        if (context_.settings.show_axes) {
            wchar_t values[192]{};
            swprintf_s(values, L"LS %.2f %.2f   RS %.2f %.2f   LT %.2f   RT %.2f", current.pad.left_stick.x,
                       current.pad.left_stick.y, current.pad.right_stick.x, current.pad.right_stick.y,
                       current.pad.left_trigger, current.pad.right_trigger);
            draw(values, font, x, y - height * (context_.settings.show_debug ? 2.7f : 1.35f), scale);
        }
        if (!first_frame_) {
            context_.logger.write("First native menu text command submitted.");
            first_frame_ = true;
        }
    } catch (const std::exception& error) {
        // Never let our C++ exception unwind through the game's frame stack.
        if (!error_logged_) {
            context_.logger.write(error.what());
            error_logged_ = true;
        }
    } catch (...) {
        if (!error_logged_) {
            context_.logger.write("Unknown menu render exception");
            error_logged_ = true;
        }
    }
}
std::string MenuController::footer_key(void* item) {
    const auto horizontal = read_field<std::uint8_t>(item, layout::item::horizontal_alignment);
    if (horizontal < 1 || horizontal > 3 ||
        read_field<std::uint8_t>(item, layout::item::vertical_alignment) != 3)
        return {};
    if (const auto text = read_field<const char*>(item, layout::item::text))
        return safe_string(text);
    const void* expression = read_field<void*>(item, layout::item::text_expression);
    if (read_field<int>(expression, 0) != 2)
        return {};
    const void* entries = read_field<void*>(expression, 8);
    // ExpressionEntry[1]: operand, string type, string pointer. Exactly
    // the constant localized labels observed in all 186 loaded menus.
    if (read_field<int>(entries, 24) != 1 || read_field<int>(entries, 32) != 2)
        return {};
    return safe_string(read_field<const char*>(entries, 40));
}
bool MenuController::footer_enabled(const Footer& footer) const {
    void* item = footer.items[0];
    if (footer.engine_key) {
        if (!context_.engine.function<int (*)(int, void*)>(item_visible_rva)(0, item))
            return false;
    } else if (!context_.engine.originals().focusable(0, item))
        return false;
    void* disabled = read_field<void*>(item, layout::item::disable_expression);
    return !disabled || !context_.engine.function<bool (*)(int, void*)>(eval_expression_rva)(0, disabled);
}
bool MenuController::dispatch_footer(void* menu, MenuAction action) {
    if (menu != footer_menu_)
        return false;
    for (const auto& footer : footers_) {
        if (footer.binding.action != action || !footer_enabled(footer))
            continue;
        void* button = footer.items[0];
        context_.logger.write("Footer action: " + footer.label);
        if (footer.engine_key) {
            native_actions().press_key(footer.engine_key);
            return true;
        }
        native_actions().run_script(button, read_field<void*>(button, layout::item::action));
        if (!context_.engine.read<int>(key_capture_rva)) {
            context_.logger.write("Footer completed: native UI capture released.");
        }
        return true;
    }
    return false;
}
NativeMenuActions MenuController::native_actions() const {
    return {context_.engine.address(menu_context_rva), context_.engine.function<ActiveMenu>(active_menu_rva),
            context_.engine.function<UiKeyEvent>(ui_key_event_rva),
            context_.engine.function<RunMenuScript>(run_script_rva)};
}
void* MenuController::current_menu() const {
    const int capture = context_.engine.read<int>(key_capture_rva);
    if (!(capture & 0x10) || (capture & 0x29))
        return nullptr; // UI owns input; console/message boxes do not.
    const int count = context_.engine.read<int>(menu_context_rva + open_menu_count_offset);
    if (count <= 0 || count > menu_stack_capacity)
        return nullptr;
    return context_.engine.function<ActiveMenu>(active_menu_rva)(context_.engine.address(menu_context_rva));
}
int MenuController::focused_index(void* menu) {
    const auto* data = read_field<void*>(menu, layout::menu::data);
    if (!data || !readable(static_cast<const std::byte*>(data) + layout::menu::focused_index, sizeof(int)))
        return -1;
    return read_field<int>(data, layout::menu::focused_index);
}
void* MenuController::focused_item(void* menu) {
    const int index = focused_index(menu), count = read_field<int>(menu, layout::menu::item_count);
    const auto* items = read_field<void*>(menu, layout::menu::items);
    if (index < 0 || index >= count || count > 1024 || !items)
        return nullptr;
    return read_field<void*>(items, static_cast<std::size_t>(index) * sizeof(void*));
}
bool MenuController::has_key_handler(void* menu, int key) {
    const auto* data = read_field<void*>(menu, layout::menu::data);
    if (!data)
        return false;
    const void* handler = read_field<void*>(data, layout::menu::key_handler);
    for (int i = 0; handler && i < 128; ++i) {
        if (!readable(handler, 24))
            break;
        if (read_field<int>(handler, 0) == key)
            return true;
        handler = read_field<void*>(handler, 16);
    }
    return false;
}
void MenuController::ensure_focus(void* menu) {
    if (context_.engine.read<int>(binding_capture_rva) || context_.engine.read<int>(edit_capture_rva))
        return;
    void* item = focused_item(menu);
    if (item && !replaced_footer(item) && (read_field<int>(item, layout::item::flags) & 6) == 6)
        return;
    if (item) {
        const auto focus = context_.engine.function<int (*)(void*, void*, float, float, int)>(focus_item_rva);
        void* context = context_.engine.address(menu_context_rva);
        const int result =
            focus(context, item, read_field<float>(context, 0x10), read_field<float>(context, 0x14), 1);
        context_.logger.write("Initial controller focus=" + std::to_string(focused_index(menu)) +
                              " accepted=" + std::to_string(result));
        if (result)
            return;
    }
    // PC menus can have an index without the actual WINDOW_HASFOCUS bit.
    // Native navigation finds the first enabled, visible interactive item.
    native_actions().press_key(155);
}
float MenuController::text_width(std::string_view text, NativeFont* font, float scale) const {
    const std::string value(text);
    return static_cast<float>(context_.engine.function<TextWidth>(text_width_rva)(value.c_str(), 0, font)) *
           scale;
}
bool MenuController::eligible(void* item) const {
    if (!readable(item, 0x1B8))
        return false;
    const auto focusable = context_.engine.function<int (*)(int, void*)>(focusable_item_rva);
    if (!focusable(0, item))
        return false;
    void* disabled = read_field<void*>(item, layout::item::disable_expression);
    return !disabled || !context_.engine.function<bool (*)(int, void*)>(eval_expression_rva)(0, disabled);
}
void MenuController::draw_icon(const char* glyphs, NativeFont* font, float x, float y, float scale,
                               const float* color) const {
    const float white[4]{1, 1, 1, 1};
    context_.engine.function<DrawText>(draw_text_rva)(glyphs, 8, font, x, y, scale, scale,
                                                      color ? color : white, 0);
}
bool MenuController::draw_row_prompt(void* item, NativeFont* font, float viewport_width,
                                     float prompt_height) {
    if (!context_.settings.row_prompts || !font || !eligible(item))
        return false;
    const int type = read_field<int>(item, layout::item::type);
    if ((type != 0 && type != 1) || !read_field<void*>(item, layout::item::action))
        return false;
    float x = read_field<float>(item, layout::item::x), y = read_field<float>(item, layout::item::y);
    float width = read_field<float>(item, layout::item::width),
          height = read_field<float>(item, layout::item::height);
    const int horizontal = read_field<std::uint8_t>(item, layout::item::horizontal_alignment),
              vertical = read_field<std::uint8_t>(item, layout::item::vertical_alignment);
    if (!std::isfinite(x) || !std::isfinite(y) || width <= 0 || height <= 0 || horizontal > 10 ||
        vertical > 10)
        return false;
    const float client_inset = read_field<float>(item, layout::item::client_x);
    const bool popup_row = inset_popup_row({x, y, width, height}, horizontal, vertical, client_inset);
    const auto placement = context_.engine.function<void* (*)()>(screen_placement_rva)();
    context_.engine.function<void (*)(void*, float*, float*, float*, float*, int, int)>(apply_rect_rva)(
        placement, &x, &y, &width, &height, horizontal, vertical);
    if (!std::isfinite(width) || !std::isfinite(height) || height < 8 || height > viewport_width ||
        width < height * 0.8f || x + width <= 0 || x + width > viewport_width + 1 || y < 0)
        return false;
    const auto lookup = context_.engine.function<Glyph* (*)(NativeFont*, unsigned)>(glyph_lookup_rva);
    const Glyph* glyph = lookup(font, 1);
    if (!glyph->height || !glyph->width)
        return false;
    // Popup hit rectangles can extend beyond their visible panel. Transform
    // both rectangles before intersecting; their native alignments differ.
    std::optional<PromptRect> clip;
    if (popup_row) {
        // This PC template adds the text inset to rect.x but leaves rect.w
        // equal to the whole panel width. rectClient.x retains that inset.
        float pad_x{}, pad_y{}, pad_width = client_inset, pad_height{};
        if (std::isfinite(pad_width) && pad_width > 0) {
            context_.engine.function<void (*)(void*, float*, float*, float*, float*, int, int)>(
                apply_rect_rva)(placement, &pad_x, &pad_y, &pad_width, &pad_height, horizontal, vertical);
            if (pad_width < width * 0.25f)
                clip = PromptRect{x - pad_width, y, width, height};
        }
    }
    void* parent = read_field<void*>(item, layout::item::parent);
    if (!clip && readable(parent, 0x24)) {
        PromptRect panel{read_field<float>(parent, 0x10), read_field<float>(parent, 0x14),
                         read_field<float>(parent, 0x18), read_field<float>(parent, 0x1C)};
        const int panel_h = read_field<std::uint8_t>(parent, 0x20),
                  panel_v = read_field<std::uint8_t>(parent, 0x21);
        if (panel.width > 0 && panel.height > 0 && panel_h <= 10 && panel_v <= 10) {
            context_.engine.function<void (*)(void*, float*, float*, float*, float*, int, int)>(
                apply_rect_rva)(placement, &panel.x, &panel.y, &panel.width, &panel.height, panel_h, panel_v);
            clip = panel;
        }
    }
    const auto position =
        row_prompt_position({x, y, width, height}, clip,
                            {static_cast<float>(glyph->x), static_cast<float>(glyph->y),
                             static_cast<float>(glyph->width), static_cast<float>(glyph->height)},
                            prompt_height);
    if (!position)
        return false;
    // PC popup hit rows extend below their visible highlight. Compensate in
    // screen pixels so the lift scales with the native row at every resolution.
    const float lift = popup_row ? height * 0.15f : 0.0f;
    draw_icon("\x01", font, position->x, position->y - lift, position->scale);
    return true;
}
void MenuController::draw_prompts(void* menu, NativeFont* font, float x, float y, float scale, float height,
                                  float viewport_width) {
    if (!prompts_.failed() && !prompts_.icons().font()) {
        try {
            if (prompts_.icons().prepare(font, context_.engine.read<void*>(cursor_material_pointer_rva)))
                context_.logger.write("Independent menu/HUD button images uploaded.");
        } catch (const std::exception& error) {
            prompts_.mark_failed();
            context_.logger.write(std::string("Xbox font unavailable; text fallback: ") + error.what());
        }
    }
    const bool capture = context_.engine.read<int>(binding_capture_rva) != 0;
    const bool editing = context_.engine.read<int>(edit_capture_rva) != 0;
    void* item = focused_item(menu);
    const int type = item ? read_field<int>(item, layout::item::type) : -1;
    const bool available = eligible(item);
    const bool confirm = !capture && !editing &&
                         ((available && (read_field<void*>(item, layout::item::action) || type == 4 ||
                                         type == 6 || type == 9 || (type >= 10 && type <= 14))) ||
                          has_key_handler(menu, 13));
    if (context_.demo)
        draw(L"[DEMO]", font, x, y, scale);
    if (confirm)
        (void)draw_row_prompt(item, prompts_.icons().font(), viewport_width, height);
    // Replace existing footer actions. Change is an explicitly requested
    // contextual hint for sliders/switches; Select is shown at the row.
    const auto placement = context_.engine.function<void* (*)()>(screen_placement_rva)();
    float right = viewport_width - x;
    const float safe_right = read_field<float>(placement, 0x40),
                safe_bottom = read_field<float>(placement, 0x44);
    if (safe_right > 0)
        right = std::min(right, safe_right);
    if (safe_bottom > 0)
        y = std::min(y, safe_bottom);
    // Native groups are stored left-to-right. Pack from the right without
    // overlaps, retaining their order and localized action labels.
    if (menu == footer_menu_)
        for (auto it = footers_.rbegin(); it != footers_.rend(); ++it)
            prompt(it->binding.glyph, it->binding.fallback, it->label, font, right, y, scale, height,
                   footer_enabled(*it));
    if (!capture && !editing && available && type >= 10 && type <= 13)
        prompt("\x16\x17", L"Left/Right", win::encode(context_.settings.change, context_.settings.code_page),
               font, right, y, scale, height, true);
}
void MenuController::prompt(const char* glyphs, std::wstring_view fallback, std::string_view label,
                            NativeFont* normal, float& right, float y, float scale, float height,
                            bool available) {
    const std::string encoded(label);
    const float label_width = text_width(encoded, normal, scale), gap = height * 0.25f;
    const float icon_scale = height / 33.0f;
    auto* icon_font = prompts_.icons().font();
    const float icon_width =
        icon_font ? text_width(glyphs, icon_font, icon_scale)
                  : text_width(win::encode(fallback, context_.settings.code_page), normal, scale);
    const float start = right - label_width - gap - icon_width;
    const float brightness = available ? 1.0f : 0.4f;
    const float color[4]{brightness, brightness, brightness, 1};
    context_.engine.function<DrawText>(draw_text_rva)(encoded.c_str(), 1024, normal, start, y, scale, scale,
                                                      color, 3);
    const float x = start + label_width + gap;
    if (auto* font = prompts_.icons().font()) {
        draw_icon(glyphs, font, x, y, icon_scale, color);
    } else {
        draw(fallback, normal, x, y, scale);
    }
    right = start - height;
}
void MenuController::draw(std::wstring_view text, void* font, float x, float y, float scale) {
    const std::string encoded = win::encode(text, context_.settings.code_page);
    const float color[4]{1.0f, 1.0f, 1.0f, 1.0f};
    const auto draw_text = context_.engine.function<DrawText>(draw_text_rva);
    // The draw function copies text into the engine's render-command buffer.
    draw_text(encoded.c_str(), 256, font, x, y, scale, scale, color, 3);
}
} // namespace mw3gf::game
