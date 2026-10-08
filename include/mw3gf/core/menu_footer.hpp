#pragma once
#include "mw3gf/core/menu_adapter.hpp"
#include <optional>
#include <string>
#include <string_view>
namespace mw3gf {
struct FooterBinding {
    MenuAction action;
    const char* glyph;
    const wchar_t* fallback;
};
// Match localization IDs, never visible English words or ordinary menu rows.
[[nodiscard]] std::optional<FooterBinding> footer_binding(std::string_view localization);
// PC shortcut assets use a colored suffix, e.g. "Friends ^2F^7".
// Keep the localized action label; remove only the keyboard/mouse suffix.
[[nodiscard]] std::string footer_label(std::string_view localized);
} // namespace mw3gf
