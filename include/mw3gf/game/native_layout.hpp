#pragma once
#include <cstddef>

namespace mw3gf::game::layout {

namespace item {
inline constexpr std::size_t name = 0;
inline constexpr std::size_t x = 8, y = 12, width = 16, height = 20;
inline constexpr std::size_t horizontal_alignment = 24, vertical_alignment = 25;
inline constexpr std::size_t client_x = 0x1C;
inline constexpr std::size_t flags = 0x50, type = 0xC4, text = 0xF0;
inline constexpr std::size_t parent = 0x100, action = 0x128;
inline constexpr std::size_t disable_expression = 0x1B0, text_expression = 0x1B8;
} // namespace item
namespace menu {
inline constexpr std::size_t data = 0, item_count = 0xB8, items = 0xC0;
inline constexpr std::size_t focused_index = 0xA8, key_handler = 0x40;
} // namespace menu
namespace client {
inline constexpr std::size_t pitch_cap = 0x4C, yaw_cap = 0x50, angles = 0x58;
inline constexpr std::size_t movement_flags = 0x2094, special_modes = 0x2098, camera_flags = 0x209C;
inline constexpr unsigned frozen_movement = 0x800, frozen_camera = 1, analog_look_modes = 0x50000;
} // namespace client
namespace command {
inline constexpr unsigned ads_button = 0x800;
inline constexpr std::size_t buttons = 4, forward = 0x1C, right = 0x1D;
inline constexpr std::size_t pitch = 0x20, yaw = 0x21, remote_yaw = 0x3E, remote_pitch = 0x3F;
} // namespace command
namespace camera {
inline constexpr unsigned forced_ads_modes = 0x1800;
inline constexpr std::size_t flags = 0xC, ads = 0x20, initialized = 0xB0, fov_scale = 0x118;
} // namespace camera
namespace text_command {
inline constexpr std::size_t size = 2, text = 0x5C, minimum_size = 0x60;
inline constexpr std::size_t maximum_size = 0xFFFF;
} // namespace text_command
namespace material {
inline constexpr std::size_t size = 0x88, texture_count = 0x56, textures = 0x68;
inline constexpr std::size_t texture_entry_size = 16, image_pointer = 8, sampler = 6;
} // namespace material
namespace image {
inline constexpr std::size_t size = 40, texture = 0, type = 8;
inline constexpr std::size_t width = 0x18, height = 0x1A, level_count = 0x1E, name = 0x20;
} // namespace image

} // namespace mw3gf::game::layout
