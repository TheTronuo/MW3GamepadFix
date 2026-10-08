#include "mw3gf/game/engine_api.hpp"
#include "mw3gf/platform/windows.hpp"
#include <cstring>
#include <stdexcept>
namespace mw3gf::game {
EngineApi::EngineApi(OriginalFunctions& originals)
    : base_(reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr))), originals_(originals) {
    if (win::sha256(win::module_path()) != supported_sha256)
        throw std::runtime_error("Unsupported iw5sp.exe SHA256; no hook installed");
    verify(game_frame_rva, game_frame_prologue, "game_frame");
    verify(create_cmd_rva, create_cmd_prologue, "create_cmd");
    verify(exec_binding_rva, exec_binding_prologue, "exec_binding");
    verify(binding_keys_rva, binding_keys_prologue, "binding_keys");
    verify(localized_text_rva, localized_text_prologue, "localized_text");
    verify(text_command_rva, text_command_prologue, "text_command");
    verify(lookup_glyph_rva, lookup_glyph_prologue, "lookup_glyph");
    verify(ui_activate_rva, ui_activate_prologue, "ui_activate");
    verify(cinematic_escape_rva, cinematic_escape_prologue, "cinematic_escape");
    verify(mouse_move_rva, mouse_move_prologue, "mouse_move");
    verify(remote_move_rva, remote_move_prologue, "remote_move");
    verify(forced_ads_rva, forced_ads_prologue, "forced_ads");
    verify(menu_paint_rva, menu_prologue, "menu_paint");
    verify(draw_text_rva, draw_text_prologue, "draw_text");
    verify(ui_key_event_rva, ui_key_prologue, "ui_key_event");
    verify(item_paint_rva, item_paint_prologue, "item_paint");
    verify(focusable_item_rva, focusable_prologue, "focusable_item");
    verify(handle_pic_rva, handle_pic_prologue, "handle_pic");
    verify(render_text_rva, render_text_prologue, "render_text");
    verify(renderer_release_rva, renderer_release_prologue, "renderer_release");
    verify(register_material_rva, register_material_prologue, "register_material");
    verify(text_width_rva, text_width_prologue, "text_width");
    verify(decoded_text_width_rva, decoded_text_width_prologue, "decoded_text_width");
    verify(text_count_rva, text_count_prologue, "text_count");
}
void EngineApi::verify(std::uintptr_t rva, std::span<const std::uint8_t> signature,
                       std::string_view name) const {
    const void* target = address(rva);
    if (!readable(target, signature.size()) || std::memcmp(target, signature.data(), signature.size()) != 0)
        throw std::runtime_error("Native signature mismatch: " + std::string(name) + "; no hook installed");
}
} // namespace mw3gf::game
