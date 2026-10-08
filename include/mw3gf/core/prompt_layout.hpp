#pragma once
#include <optional>
namespace mw3gf {
struct PromptRect {
    float x{}, y{}, width{}, height{};
};
struct PromptGlyph {
    float x{}, y{}, width{}, height{};
};
struct PromptPosition {
    float x{}, y{}, scale{};
};
// Identify the shared PC dialog row before applying native screen placement.
[[nodiscard]] bool inset_popup_row(PromptRect row, int horizontal, int vertical, float client_inset) noexcept;
// All rectangles are in final screen pixels, after native screen placement.
std::optional<PromptPosition> row_prompt_position(PromptRect row, std::optional<PromptRect> clip,
                                                  PromptGlyph glyph, float prompt_height) noexcept;
} // namespace mw3gf
