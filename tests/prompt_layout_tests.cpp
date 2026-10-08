#include "mw3gf/core/prompt_layout.hpp"
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace
int main() {
    try {
        using namespace mw3gf;
        // Captured templates: Resume and SWF confirmations use the same geometry
        // despite different item names. Dialog widths vary with their contents.
        for (const float width : std::array{155.0f, 260.0f, 320.0f, 330.0f, 360.0f, 460.0f}) {
            require(inset_popup_row({11.667f, 0, width, 22}, 2, 2, 11.667f),
                    "Shared inset popup rows receive the alignment correction");
        }
        require(inset_popup_row({-122, 33.667f, 260, 22}, 2, 2, 11.667f),
                "Repositioned open dialog retains the popup correction");
        require(!inset_popup_row({0, 0, 260, 22}, 2, 2, 0), "An ordinary centered row is not an inset popup");
        require(!inset_popup_row({11.667f, 0, 260, 22}, 1, 2, 11.667f),
                "Other native alignments retain their layout");
        require(!inset_popup_row({11.667f, 0, 260, 44}, 2, 2, 11.667f),
                "Cards and larger rows retain their layout");
        require(!inset_popup_row({11.667f, 0, std::numeric_limits<float>::quiet_NaN(), 22}, 2, 2, 11.667f),
                "Malformed geometry cannot select the popup correction");
        for (const float resolution_scale : std::array{1.0f, 1.5f, 3.0f}) {
            const PromptRect popup{379 * resolution_scale, 280 * resolution_scale, 565 * resolution_scale,
                                   157 * resolution_scale};
            const PromptRect hit{379 * resolution_scale, 378 * resolution_scale, 585 * resolution_scale,
                                 45 * resolution_scale};
            const PromptGlyph glyph{2, -33, 32, 32};
            const auto p = row_prompt_position(hit, popup, glyph, 32 * resolution_scale);
            require(p.has_value(), "Popup row gets a prompt");
            require(p->x + (glyph.x + glyph.width) * p->scale < popup.x + popup.width,
                    "A remains inside visible popup, including its bearing");
            require(p->y + glyph.y * p->scale >= hit.y, "Top stays in row");
            require(p->y + (glyph.y + glyph.height) * p->scale <= hit.y + hit.height, "Bottom stays in row");
            // Captured PC popup template: 260-wide panel, 11.667 text inset,
            // 22-high row. Its hit rect retains the full panel width.
            const PromptRect swf_hit{(320 - 130 + 11.667f) * resolution_scale, 200 * resolution_scale,
                                     260 * resolution_scale, 22 * resolution_scale};
            const PromptRect swf_panel{(320 - 130) * resolution_scale, 200 * resolution_scale,
                                       260 * resolution_scale, 22 * resolution_scale};
            const auto swf = row_prompt_position(swf_hit, swf_panel, glyph, 32 * resolution_scale);
            require(swf && swf->x + (glyph.x + glyph.width) * swf->scale < swf_panel.x + swf_panel.width,
                    "SWF popup text inset must not push A beyond its panel");
        }
        const auto card = row_prompt_position({200, 100, 260, 220}, std::nullopt, {0, -33, 32, 32}, 30);
        // Other captured popup families use lists, compact Yes/No buttons or
        // selectable cards. They retain their own placement and stay in bounds.
        constexpr std::array popup_sizes{PromptRect{0, 0, 292, 20},          PromptRect{0, 0, 60, 30},
                                         PromptRect{0, 0, 305.333f, 20},     PromptRect{0, 0, 50, 20},
                                         PromptRect{0, 0, 46.667f, 46.667f}, PromptRect{0, 0, 48, 48},
                                         PromptRect{0, 0, 164, 36}};
        for (const auto size : popup_sizes) {
            for (const float resolution_scale : std::array{1.0f, 1.5f, 3.0f}) {
                const PromptRect row{100 * resolution_scale, 100 * resolution_scale,
                                     size.width * resolution_scale, size.height * resolution_scale};
                const PromptGlyph glyph{2, -33, 32, 32};
                const auto position = row_prompt_position(row, std::nullopt, glyph, 32 * resolution_scale);
                require(position.has_value(), "Other popup row families retain their prompts");
                const float left = position->x + glyph.x * position->scale;
                const float top = position->y + glyph.y * position->scale;
                require(left >= row.x && left + glyph.width * position->scale <= row.x + row.width &&
                            top >= row.y && top + glyph.height * position->scale <= row.y + row.height,
                        "Popup lists, cards and compact buttons keep the icon within their row");
            }
        }
        require(card && card->y - 33 * card->scale > 270, "Card prompt remains at bottom");
        require(!row_prompt_position({0, 0, 100, 30}, PromptRect{200, 0, 100, 30}, {0, -33, 32, 32}, 30),
                "No prompt outside clipping bounds");
        require(!row_prompt_position({0, 0, 100, 30}, std::nullopt,
                                     {0, 0, 32, std::numeric_limits<float>::quiet_NaN()}, 30),
                "Invalid glyph is rejected");
        std::cout << "Prompt bounds verified\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
