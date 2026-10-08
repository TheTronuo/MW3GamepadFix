#pragma once
#include <optional>
#include <string>
#include <string_view>
namespace mw3gf {
// Camera prompts can already show the reserved right stick; input stays stage 3.
std::optional<unsigned> action_prompt_glyph(std::string_view action) noexcept;
// Replace explicit input tokens in localized instructions, preserving language.
std::string controller_instruction(std::string_view key, std::string_view original);
std::string expand_action_prompts(std::string_view text);
bool contains_prompt_glyph(std::string_view text) noexcept;
// Keep textured buttons white, restoring the surrounding engine text color.
std::size_t inline_material_bytes(std::string_view text, std::size_t at) noexcept;
unsigned image_dimension_byte(unsigned pixels) noexcept;
int image_dimension(unsigned encoded, int font_height) noexcept;
std::string inline_button_record(unsigned code, unsigned width, unsigned height);
using PromptReadCharacter = unsigned (*)(const char**, void*);
using PromptCharacterAdvance = int (*)(unsigned, void*);
// Same visible-item limits and decoder as the game's width routine, with
// native material records treated atomically instead of as their name bytes.
std::optional<int> prompt_text_width(std::string_view text, int max_chars, int font_height,
                                     PromptReadCharacter read, PromptCharacterAdvance advance, void* context);
std::optional<int> prompt_visible_characters(std::string_view text, PromptReadCharacter read, void* context);
} // namespace mw3gf
