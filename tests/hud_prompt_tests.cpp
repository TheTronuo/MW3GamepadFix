#include "mw3gf/game/hud_prompt.hpp"
#include "mw3gf/core/gameplay_prompts.hpp"
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
    using namespace mw3gf;
    try {
        require(is_hud_prompt_lookup(27, sdv_prompt_key, 0x289B7F), "Recognize direct HUD lookup");
        require(!is_hud_prompt_lookup(26, sdv_prompt_key, 0x289B7F) &&
                    !is_hud_prompt_lookup(27, sdv_prompt_key, 0x28A6B0) &&
                    !is_hud_prompt_lookup(27, "MENU_MOUSE_LOOK", 0x289B7F),
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
            require(cache.lookup(&asset, sdv_prompt_key, text, text) == &asset,
                    "An unchanged instruction returns the database entry");
            const auto* view = cache.lookup(&asset, sdv_prompt_key, text, controller_instruction(sdv_prompt_key, text));
            require(view != &asset && std::string(view->value) == expected,
                    "Native HUD receives both LS and RS while preserving the locale");
            require(asset.value == text.c_str() && std::string(asset.value) == text,
                    "Database asset and localization bytes never change");
            require(std::string(view->name) == sdv_prompt_key &&
                        cache.lookup(&asset, sdv_prompt_key, text, expected) == view,
                    "Replacement header and value have stable ownership");
            require(cache.lookup(view, sdv_prompt_key, view->value, view->value) == view,
                    "A previously converted entry remains idempotent");
            if (!first)
                first = view;
        }
        require(first && std::string(first->value) == locales[0].second,
                "Locale changes and later insertions preserve earlier returned pointers");
        NativeLocalizeEntry other{"^3Мышь^7", "OTHER"};
        require(cache.lookup(&other, "OTHER", other.value, other.value) == &other &&
                    cache.lookup(nullptr, sdv_prompt_key, other.value, other.value) == nullptr,
                "Unrelated and missing assets are preserved");
        const HoldTextTemplates templates{
            {"Press ^3[{+activate}]^7 to secure the enemy intelligence.", "Press ^3 [{+activate}] ^7 to breach"},
            {"Hold^3 &&1 ^7to plant explosives", "Hold^3 &&1 ^7to defuse explosives"}};
        for (const auto& rule : use_hold_rules()) {
            for (const auto key : rule.prompt_keys) {
                if (key.empty()) continue;
                require(is_hud_prompt_lookup(27, key, hud_localize_return_rva) &&
                            !is_hud_prompt_lookup(27, key, 0x28A6B0), "Hold catalog keys have audited HUD routing");
            }
        }
        const auto key = std::string_view("SCRIPT_PLATFORM_BREACH_ACTIVATE");
        const auto text = templates.press[1];
        NativeLocalizeEntry breach{text.data(), key.data()};
        const auto replacement = controller_instruction(key, localized_hold_instruction(text, templates));
        const auto* converted = cache.lookup(&breach, key, text, replacement);
        require(converted != &breach && std::string(converted->value) == "Hold^3 \x03 ^7 to breach" &&
                    breach.value == text.data(), "Script HUD receives Hold plus X without mutating its asset");
        require(cache.lookup(&breach, key, text, std::string(text)) == &breach,
                "Prior controller conversion does not leak into original input mode");
        const auto* revised = cache.lookup(&breach, key, text, "another replacement");
        require(revised != converted && std::string(converted->value) == replacement,
                "Different conversion policies retain both returned views");
        const auto weapon_key = std::string_view("PLATFORM_SWAPWEAPONS");
        const auto* weapon_rule = find_use_hold_prompt(weapon_key);
        const HoldTextTemplates retail_weapon{
            {"^3 &&1 ^7- обменять на", "Нажмите^3 &&1 ^7, чтобы взять"},
            {"Нажать и удерживать ^3 &&1 ^7- установить взрывчатку",
             "Нажать и удерживать ^3 &&1 ^7- обезвредить взрывчатку"},
            weapon_rule->allow_implicit_prefix};
        NativeLocalizeEntry weapon{retail_weapon.press[0].data(), weapon_key.data()};
        const auto* weapon_view = cache.lookup(
            &weapon, weapon_key, weapon.value,
            controller_instruction(weapon_key, localized_hold_instruction(weapon.value, retail_weapon)));
        require(is_hud_prompt_lookup(27, weapon_key, hud_localize_return_rva) &&
                    weapon_view != &weapon && std::string(weapon_view->value) ==
                        "Нажать и удерживать ^3 &&1 ^7- обменять на" &&
                    std::string(weapon.value) == retail_weapon.press[0],
                "Russian retail HUD gains Hold and preserves the native button parameter and asset");
        std::cout << "Native HUD entry routing, locale preservation, input gating and lifetime verified\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
