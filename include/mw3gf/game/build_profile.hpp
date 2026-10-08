#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace mw3gf::game {
// Static analysis of this exact iw5sp.exe; all addresses are module-relative RVAs.
inline constexpr std::string_view supported_sha256 =
    "a97d2bbc7e495e4cf1a3b7e9a021e25b2c7b461d63f2b88d3cfe8782e8dc1023";
inline constexpr std::uintptr_t menu_paint_rva = 0x29D170;
inline constexpr std::uintptr_t draw_text_rva = 0x080840;
inline constexpr std::uintptr_t menu_context_rva = 0x2605050;
inline constexpr std::uintptr_t normal_font_pointer_rva = 0x2604FA8;
inline constexpr std::uintptr_t ui_key_event_rva = 0x29BAA0;
inline constexpr std::uintptr_t active_menu_rva = 0x2AAA80;
inline constexpr std::uintptr_t key_capture_rva = 0x6E2550;
inline constexpr std::uintptr_t binding_capture_rva = 0x2615F80;
inline constexpr std::uintptr_t edit_capture_rva = 0x2615F84;
inline constexpr std::uintptr_t glyph_lookup_rva = 0x1B7BF0;
inline constexpr std::uintptr_t focus_item_rva = 0x2A7F00;
inline constexpr std::uintptr_t focusable_item_rva = 0x2B1DE0;
inline constexpr std::uintptr_t eval_expression_rva = 0x28EB80;
inline constexpr std::uintptr_t screen_placement_rva = 0x8D8F0;
inline constexpr std::uintptr_t apply_rect_rva = 0x8CEF0;
inline constexpr std::uintptr_t item_paint_rva = 0x2A7660;
inline constexpr std::uintptr_t item_visible_rva = 0x2B21B0;
inline constexpr std::uintptr_t handle_pic_rva = 0x28C2B0;
inline constexpr std::uintptr_t cursor_material_pointer_rva = 0x2604F58;
inline constexpr std::uintptr_t render_text_rva = 0x188DF0;
inline constexpr std::uintptr_t renderer_release_rva = 0x1BD730;
inline constexpr std::uintptr_t register_material_rva = 0x1C4B80;
inline constexpr std::uintptr_t text_width_rva = 0x1B8020;
// HUD pickup prompts use this optimized reader, which bypasses glyph lookup.
inline constexpr std::uintptr_t decoded_text_width_rva = 0x1B80F0;
inline constexpr std::uintptr_t read_character_rva = 0x28A010;
inline constexpr std::uintptr_t text_count_rva = 0x289E80;
inline constexpr std::array<std::uint8_t, 16> text_count_prologue{
    0x48, 0x83, 0xEC, 0x08, 0x48, 0x8B, 0xD1, 0x48, 0x85, 0xC9, 0x75, 0x07, 0x33, 0xC0, 0x48, 0x83};
inline constexpr std::array<std::uint8_t, 16> register_material_prologue{
    0x41, 0xB8, 0x01, 0, 0, 0, 0x48, 0x8B, 0xD1, 0x41, 0x8D, 0x48, 0x04, 0xE9, 0x8E, 0x0E};
inline constexpr std::array<std::uint8_t, 16> text_width_prologue{
    0x48, 0x89, 0x6C, 0x24, 0x18, 0x48, 0x89, 0x74, 0x24, 0x20, 0x57, 0x41, 0x56, 0x41, 0x57, 0x48};
inline constexpr std::array<std::uint8_t, 16> decoded_text_width_prologue{
    0x48, 0x89, 0x6C, 0x24, 0x20, 0x48, 0x89, 0x4C, 0x24, 0x08, 0x56, 0x41, 0x54, 0x41, 0x55, 0x41};
inline constexpr std::array<std::uint8_t, 15> handle_pic_prologue{
    0x48, 0x8B, 0xC4, 0x48, 0x81, 0xEC, 0xA8, 0x00, 0x00, 0x00, 0xF3, 0x0F, 0x10, 0x2D, 0xCE};
inline constexpr std::array<std::uint8_t, 15> render_text_prologue{
    0x48, 0x8B, 0xC4, 0x48, 0x89, 0x58, 0x08, 0x57, 0x48, 0x81, 0xEC, 0xD0, 0x00, 0x00, 0x00};
inline constexpr std::array<std::uint8_t, 15> renderer_release_prologue{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18};
inline constexpr std::uintptr_t localize_rva = 0x29F120;
inline constexpr std::uintptr_t run_script_rva = 0x2A3CA0;
inline constexpr std::array<std::uint8_t, 15> item_paint_prologue{
    0x40, 0x56, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x48, 0x81, 0xEC, 0x18, 0x02, 0x00, 0x00};
inline constexpr std::array<std::uint8_t, 15> focusable_prologue{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x20};
using ItemPaint = void (*)(void* context, void* placement, void* item);
using FocusableItem = int (*)(int local_client, void* item);
inline constexpr std::uintptr_t menu_stack_offset = 0x1440;
// +0x1438 counts loaded menu definitions (186 here), NOT the open stack.
// UI's stack membership function at RVA 0x2AD530 reads +0x14C0 and
// walks 16 menu pointers starting at +0x1440.
inline constexpr std::uintptr_t open_menu_count_offset = 0x14C0;
inline constexpr int menu_stack_capacity = 16;
inline constexpr std::array<std::uint8_t, 14> menu_prologue{0x40, 0x53, 0x55, 0x41, 0x56, 0x41, 0x57,
                                                            0x48, 0x81, 0xEC, 0xA8, 0x00, 0x00, 0x00};
inline constexpr std::array<std::uint8_t, 16> draw_text_prologue{
    0x48, 0x83, 0xEC, 0x58, 0xF3, 0x0F, 0x10, 0x8C, 0x24, 0x90, 0x00, 0x00, 0x00, 0x0F, 0x57, 0xC0};
inline constexpr std::array<std::uint8_t, 15> ui_key_prologue{0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89, 0x74,
                                                              0x24, 0x18, 0x57, 0x48, 0x83, 0xEC, 0x20};
// The game wrapper supplies zero rotation to its lower-level draw command.
using MenuPaint = void (*)(int local_client);
using UiKeyEvent = void (*)(int local_client, int engine_key, int down);
using ActiveMenu = void* (*)(void* context);
using DrawText = void (*)(const char* text, int max_chars, void* font, float x, float y, float x_scale,
                          float y_scale, const float* color, int style);
} // namespace mw3gf::game
