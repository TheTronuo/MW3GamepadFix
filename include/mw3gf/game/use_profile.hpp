#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace mw3gf::game {
inline constexpr std::uintptr_t entity_use_rva = 0x176FF0;
inline constexpr std::uintptr_t timed_entity_use_return_rva = 0x175F3A;
inline constexpr std::uintptr_t server_time_rva = 0x1140C88;
inline constexpr std::uintptr_t usereload_button_rva = 0x6448A4;
inline constexpr int controller_use_owner = 0x4005;
inline constexpr std::uintptr_t weapon_class_rva = 0x26D40;
inline constexpr std::uintptr_t server_config_string_rva = 0x26ACD0;
inline constexpr std::array<std::uint8_t, 16> entity_use_prologue{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x8B, 0xF9, 0x48, 0x8B, 0xDA};
inline constexpr std::array<std::uint8_t, 16> weapon_class_prologue{
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x40, 0x4C, 0x8D, 0x0D, 0xB3, 0x92, 0xFD, 0xFF, 0x84, 0xD2, 0x74};
inline constexpr std::array<std::uint8_t, 16> server_config_string_prologue{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x63, 0xC1, 0x41, 0x8B, 0xD8};
namespace use_layout {
inline constexpr std::size_t entity_type = 0, hint_index = 2, weapon = 0x7C, entity_number = 0x84;
inline constexpr std::size_t cursor_hint = 0xAC, client = 0x110;
inline constexpr std::size_t command = 0xAD58, buttons = 0xAE08;
inline constexpr std::size_t use_handle = 0xAEE8, use_start = 0xAEEC;
inline constexpr std::uint32_t usereload = 0x20;
inline constexpr int hint_config_base = 0x92, hint_config_count = 32;
} // namespace use_layout
} // namespace mw3gf::game
