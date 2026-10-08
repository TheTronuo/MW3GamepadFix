#include "mw3gf/core/menu_footer.hpp"
namespace mw3gf {
std::optional<FooterBinding> footer_binding(std::string_view localization) {
    if (localization == "@MENU_QUIT" || localization == "@MENU_EXIT" ||
        localization == "@PLATFORM_BACK_SHORTCUT")
        return FooterBinding{MenuAction::back, "\x02", L"B"};
    if (localization == "@PLATFORM_FRIENDS_SHORTCUT")
        return FooterBinding{MenuAction::auxiliary_y, "\x04", L"Y"};
    if (localization == "@PLATFORM_LEADERBOARDS_SHORTCUT")
        return FooterBinding{MenuAction::auxiliary_x, "\x03", L"X"};
    if (localization == "@PLATFORM_GAMESUMMARY_SHORTCUT")
        return FooterBinding{MenuAction::auxiliary_view, "\x0f", L"Back"};
    if (localization == "@MENU_ACCEPT")
        return FooterBinding{MenuAction::confirm, "\x01", L"A"};
    if (localization == "@PLATFORM_FILTER_SHORTCUT")
        return FooterBinding{MenuAction::auxiliary_y, "\x04", L"Y"};
    if (localization == "@PLATFORM_PAGE_UP_SHORTCUT")
        return FooterBinding{MenuAction::page_up, "\x05", L"LB"};
    if (localization == "@PLATFORM_PAGE_DOWN_SHORTCUT")
        return FooterBinding{MenuAction::page_down, "\x06", L"RB"};
    if (localization == "@PLATFORM_TOP_SHORTCUT")
        return FooterBinding{MenuAction::top, "\x10", L"LS"};
    if (localization == "@PLATFORM_BOTTOM_SHORTCUT")
        return FooterBinding{MenuAction::bottom, "\x11", L"RS"};
    return std::nullopt;
}
std::string footer_label(std::string_view localized) {
    // These specific shortcut resources put their key after ^2. Other
    // formatting is retained; do not truncate unrecognized/menu body text.
    if (const auto suffix = localized.find("^2"); suffix != std::string_view::npos)
        localized = localized.substr(0, suffix);
    while (!localized.empty() && localized.back() == ' ')
        localized.remove_suffix(1);
    return std::string(localized);
}
} // namespace mw3gf
