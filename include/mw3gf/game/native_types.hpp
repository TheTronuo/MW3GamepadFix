#pragma once
#include <cstddef>
#include <cstdint>
namespace mw3gf::game {
struct Glyph {
    std::uint16_t code;
    std::int8_t x, y;
    std::uint8_t advance, width, height, padding;
    float s0, t0, s1, t1;
};
struct NativeFont {
    const char* name;
    int pixel_height, glyph_count;
    void* material;
    void* glow_material;
    Glyph* glyphs;
};
static_assert(sizeof(Glyph) == 24 && sizeof(NativeFont) == 40);
static_assert(offsetof(Glyph, advance) == 4 && offsetof(Glyph, s0) == 8);
static_assert(offsetof(NativeFont, material) == 16 && offsetof(NativeFont, glyphs) == 32);
} // namespace mw3gf::game
