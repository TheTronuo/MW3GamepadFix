#include "native_hooks.hpp"
#include "mw3gf/platform/windows.hpp"
#include <intrin.h>
namespace mw3gf::game::detail {
namespace {
void hooked_menu_paint(int local_client) {
    if (enabled.load(std::memory_order_acquire))
        runtime->menu().begin_footer_frame(local_client);
    originals.menu_paint(local_client);
    if (enabled.load(std::memory_order_acquire))
        runtime->menu().paint(local_client);
}
void hooked_item_paint(void* context, void* placement, void* item) {
    if (enabled.load(std::memory_order_acquire) && runtime->menu().replaced_footer(item))
        return;
    originals.item_paint(context, placement, item);
}
int hooked_focusable(int local_client, void* item) {
    if (local_client == 0 && enabled.load(std::memory_order_acquire) && runtime->menu().replaced_footer(item))
        return 0;
    return originals.focusable(local_client, item);
}
void hooked_game_frame(int msec, float fraction) {
    runtime->gameplay().pump_gameplay(enabled.load(std::memory_order_acquire));
    // Menu callbacks can synchronously shut down the renderer. Dispatch them
    // from the client frame before drawing, never from UI painting.
    if (enabled.load(std::memory_order_acquire))
        runtime->menu().pump_input();
    originals.game_frame(msec, fraction);
}
void* hooked_create_cmd(void* command, int local_client) {
    void* result = originals.create_cmd(command, local_client);
    if (enabled.load(std::memory_order_acquire))
        runtime->gameplay().add_movement(result, local_client);
    return result;
}
void hooked_mouse_move(void* command, float seconds) {
    if (enabled.load(std::memory_order_acquire))
        runtime->gameplay().add_camera(command, seconds);
    originals.mouse_move(command, seconds);
}
void hooked_remote_move(int local_client, void* command) {
    originals.remote_move(local_client, command);
    if (enabled.load(std::memory_order_acquire))
        runtime->gameplay().add_remote_stick(local_client, command);
}
int hooked_binding_keys(int client, const char* command, char* output) {
    if (enabled.load(std::memory_order_acquire)) {
        const int count = runtime->prompts().binding_keys(client, command, output);
        if (count >= 0)
            return count;
    }
    return originals.binding_keys(client, command, output);
}
const char* hooked_localized_text(const char* key) {
    const char* original = originals.localized_text(key);
    return enabled.load(std::memory_order_acquire) ? runtime->prompts().localized(key, original) : original;
}
void* hooked_find_asset(int type, const char* key, int flags) {
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    void* original = originals.find_asset(type, key, flags);
    if (enabled.load(std::memory_order_acquire) && type == localize_asset_type &&
        caller == runtime->context().engine.base() + hud_localize_return_rva)
        return runtime->prompts().hud_localized_asset(type, key, original, hud_localize_return_rva);
    return original;
}
void* hooked_text_command(const char* text, int max_chars, NativeFont* font, float x, float y, float xs,
                          float ys, float rotation, const float* color, int style, int cursor, bool flag) {
    if (enabled.load(std::memory_order_acquire))
        return runtime->prompts().text_command(text, max_chars, font, x, y, xs, ys, rotation, color, style,
                                               cursor, flag);
    return originals.text_command(text, max_chars, font, x, y, xs, ys, rotation, color, style, cursor, flag);
}
Glyph* hooked_lookup_glyph(NativeFont* font, unsigned code) {
    return enabled.load(std::memory_order_acquire) ? runtime->prompts().glyph(font, code)
                                                   : originals.lookup_glyph(font, code);
}
void hooked_handle_pic(void* placement, float x, float y, float width, float height, int horizontal,
                       int vertical, const float* color, void* material) {
    if (enabled.load(std::memory_order_acquire) && runtime->menu().hide_cursor(material))
        return;
    originals.handle_pic(placement, x, y, width, height, horizontal, vertical, color, material);
}
void hooked_render_text(void* state) {
    // Finish already queued button markers even after Stop/disconnect.
    runtime->prompts().render_text(state);
}
void* hooked_register_material(const char* name, int flags) {
    if (void* material = runtime->prompts().prompt_material(name))
        return material;
    return originals.register_material(name, flags);
}
int hooked_text_width(const char* text, int max_chars, NativeFont* font) {
    return enabled.load(std::memory_order_acquire) ? runtime->prompts().measured_width(text, max_chars, font)
                                                   : originals.text_width(text, max_chars, font);
}
int hooked_decoded_text_width(const char* text, int max_chars, NativeFont* font, int decode_mode) {
    return enabled.load(std::memory_order_acquire)
               ? runtime->prompts().measured_width(text, max_chars, font, decode_mode)
               : originals.decoded_text_width(text, max_chars, font, decode_mode);
}
int hooked_text_count(const char* text) {
    return runtime->prompts().visible_characters(text);
}
void hooked_renderer_release(int mode) {
    runtime->prompts().renderer_release();
    originals.renderer_release(mode);
}
} // namespace
void install_hooks(ModRuntime& instance) {
    auto& hooks = instance.hooks();
    const auto& engine = instance.context().engine;
    hooks.add(HookId::menu_paint, engine.address(menu_paint_rva), &hooked_menu_paint, originals.menu_paint);
    hooks.add(HookId::item_paint, engine.address(item_paint_rva), &hooked_item_paint, originals.item_paint);
    hooks.add(HookId::focusable, engine.address(focusable_item_rva), &hooked_focusable, originals.focusable);
    hooks.add(HookId::game_frame, engine.address(game_frame_rva), &hooked_game_frame, originals.game_frame);
    hooks.add(HookId::create_cmd, engine.address(create_cmd_rva), &hooked_create_cmd, originals.create_cmd);
    hooks.add(HookId::binding_keys, engine.address(binding_keys_rva), &hooked_binding_keys,
              originals.binding_keys);
    hooks.add(HookId::localized_text, engine.address(localized_text_rva), &hooked_localized_text,
              originals.localized_text);
    hooks.add(HookId::find_asset, engine.address(find_asset_rva), &hooked_find_asset, originals.find_asset);
    hooks.add(HookId::text_command, engine.address(text_command_rva), &hooked_text_command,
              originals.text_command);
    hooks.add(HookId::lookup_glyph, engine.address(lookup_glyph_rva), &hooked_lookup_glyph,
              originals.lookup_glyph);
    hooks.add(HookId::mouse_move, engine.address(mouse_move_rva), &hooked_mouse_move, originals.mouse_move);
    hooks.add(HookId::remote_move, engine.address(remote_move_rva), &hooked_remote_move,
              originals.remote_move);
    hooks.add(HookId::handle_pic, engine.address(handle_pic_rva), &hooked_handle_pic, originals.handle_pic);
    hooks.add(HookId::render_text, engine.address(render_text_rva), &hooked_render_text,
              originals.render_text);
    hooks.add(HookId::renderer_release, engine.address(renderer_release_rva), &hooked_renderer_release,
              originals.renderer_release);
    hooks.add(HookId::register_material, engine.address(register_material_rva), &hooked_register_material,
              originals.register_material);
    hooks.add(HookId::text_width, engine.address(text_width_rva), &hooked_text_width, originals.text_width);
    hooks.add(HookId::decoded_text_width, engine.address(decoded_text_width_rva), &hooked_decoded_text_width,
              originals.decoded_text_width);
    hooks.add(HookId::text_count, engine.address(text_count_rva), &hooked_text_count, originals.text_count);
}
void pin_hook_module() {
    HMODULE pinned{};
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                            reinterpret_cast<LPCWSTR>(&hooked_menu_paint), &pinned))
        win::fail_last_error("Pin module");
}
} // namespace mw3gf::game::detail
