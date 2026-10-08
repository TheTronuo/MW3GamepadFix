#include "mw3gf/core/prompt_layout.hpp"
#include <algorithm>
#include <cmath>
namespace mw3gf {
namespace {
bool valid(PromptRect rect) {
    return std::isfinite(rect.x) && std::isfinite(rect.y) && std::isfinite(rect.width) &&
           std::isfinite(rect.height) && rect.width > 0 && rect.height > 0;
}
} // namespace
bool inset_popup_row(PromptRect row, int horizontal, int vertical, float client_inset) noexcept {
    // The native popup template shares this text inset and row height across
    // Resume, save confirmations, filters and other dialogs with distinct names.
    constexpr float text_inset = 11.667f;
    constexpr float row_height = 22.0f;
    constexpr float tolerance = 0.01f;
    // Scripts reposition rect.x/rect.y after opening; rectClient.x retains the inset.
    return valid(row) && horizontal == 2 && vertical == 2 &&
           std::abs(client_inset - text_inset) < tolerance && std::abs(row.height - row_height) < tolerance &&
           row.width > row.height * 2;
}
std::optional<PromptPosition> row_prompt_position(PromptRect row, std::optional<PromptRect> clip,
                                                  PromptGlyph glyph, float prompt_height) noexcept {
    if (!valid(row) || !std::isfinite(prompt_height) || prompt_height <= 0 || !std::isfinite(glyph.x) ||
        !std::isfinite(glyph.y) || !std::isfinite(glyph.width) || !std::isfinite(glyph.height) ||
        glyph.width <= 0 || glyph.height <= 0)
        return std::nullopt;
    if (clip && valid(*clip)) {
        const float right = std::min(row.x + row.width, clip->x + clip->width);
        const float bottom = std::min(row.y + row.height, clip->y + clip->height);
        row.x = std::max(row.x, clip->x);
        row.y = std::max(row.y, clip->y);
        row.width = right - row.x;
        row.height = bottom - row.y;
    }
    if (!valid(row) || row.height < 8 || row.width < row.height * 0.8f)
        return std::nullopt;
    const bool card = row.width < row.height * 2;
    const float preferred = card ? std::min(row.height * 0.2f, prompt_height) : row.height * 0.82f;
    const float inset = (card ? preferred : row.height) * 0.12f;
    const float scale = std::min({preferred / glyph.height, (row.width - 2 * inset) / glyph.width,
                                  (row.height - 2 * inset) / glyph.height});
    if (scale <= 0)
        return std::nullopt;
    const float center_y =
        card ? row.y + row.height - inset - glyph.height * scale * 0.5f : row.y + row.height * 0.5f;
    return PromptPosition{row.x + row.width - inset - (glyph.width + glyph.x) * scale,
                          center_y - (glyph.y + glyph.height * 0.5f) * scale, scale};
}
} // namespace mw3gf
