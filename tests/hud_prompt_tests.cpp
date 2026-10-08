#include "mw3gf/game/hud_prompt.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
void require(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
} // namespace
int main() {
    using namespace mw3gf::game;
    try {
        require(is_sdv_hud_lookup(27, sdv_prompt_key, 0x289B7F), "Recognize direct HUD lookup");
        require(!is_sdv_hud_lookup(26, sdv_prompt_key, 0x289B7F) &&
                    !is_sdv_hud_lookup(27, sdv_prompt_key, 0x28A6B0) &&
                    !is_sdv_hud_lookup(27, "MENU_MOUSE_LOOK", 0x289B7F),
                "Unrelated asset types, callers and keys remain untouched");
        HudPromptCache cache;
        const std::vector<std::pair<std::string, std::string>> locales{
            {"Нажимайте ^3[{+forward}]^7 для ускорения, используйте  ^3Мышь^7  для управления",
             "Нажимайте ^3\x10^7 для ускорения, используйте  ^3\x11^7  для управления"},
            {"Push ^3[{+forward}]^7 to accelerate and ^3mouse^7 to steer",
             "Push ^3\x10^7 to accelerate and ^3\x11^7 to steer"},
            {"^3[{+forward}]^7 تسارع ^3الفأرة^7 توجيه", "^3\x10^7 تسارع ^3\x11^7 توجيه"},
            {"^3\x10^7 accelerate ^3unbekannt^7 steer", "^3\x10^7 accelerate ^3\x11^7 steer"},
        };
        const NativeLocalizeEntry* first{};
        for (const auto& [text, expected] : locales) {
            NativeLocalizeEntry asset{text.c_str(), sdv_prompt_key.data()};
            require(cache.lookup(&asset, sdv_prompt_key, text, false) == &asset,
                    "Keyboard/disconnected mode uses the database entry");
            const auto* view = cache.lookup(&asset, sdv_prompt_key, text, true);
            require(view != &asset && std::string(view->value) == expected,
                    "Native HUD receives both LS and RS while preserving the locale");
            require(asset.value == text.c_str() && std::string(asset.value) == text,
                    "Database asset and localization bytes never change");
            require(std::string(view->name) == sdv_prompt_key &&
                        cache.lookup(&asset, sdv_prompt_key, text, true) == view,
                    "Replacement header and value have stable ownership");
            require(cache.lookup(view, sdv_prompt_key, view->value, true) == view,
                    "A previously converted entry remains idempotent");
            if (!first)
                first = view;
        }
        require(first && std::string(first->value) == locales[0].second,
                "Locale changes and later insertions preserve earlier returned pointers");
        NativeLocalizeEntry other{"^3Мышь^7", "OTHER"};
        require(cache.lookup(&other, "OTHER", other.value, true) == &other &&
                    cache.lookup(nullptr, sdv_prompt_key, other.value, true) == nullptr,
                "Unrelated and missing assets are preserved");
        std::cout << "Native HUD entry routing, locale preservation, input gating and lifetime verified\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
