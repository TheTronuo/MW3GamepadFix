#include "mw3gf/game/hud_prompt.hpp"
#include "mw3gf/core/gameplay_prompts.hpp"
#include "mw3gf/core/button_icon.hpp"
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
        const HoldTextTemplates compact_weapon{retail_weapon.press,
            {"Удерживайте^3 &&1 ^7чтобы лечь", "Удерживайте^3 &&1 ^7чтобы пригибаться."}, true};
        const auto compact_value = localized_weapon_hold_instruction(weapon_key, weapon.value, compact_weapon);
        require(compact_value.has_value(), "Compatible compact weapon templates are available");
        const auto* compact_view = cache.lookup(&weapon, weapon_key, weapon.value, *compact_value);
        require(compact_view != weapon_view && compact_view != &weapon &&
                    std::string(compact_view->value) == "Удерживайте^3 &&1 ^7, чтобы взять" &&
                    std::string(weapon_view->value) == "Нажать и удерживать ^3 &&1 ^7- обменять на" &&
                    std::string(weapon.value) == retail_weapon.press[0] &&
                    cache.lookup(&weapon, weapon_key, weapon.value, std::string(weapon.value)) == &weapon,
                "Compact HUD wording keeps both cached pointers, original asset and keyboard path intact");
        const std::vector<std::string> prone_locales{
            "Удерживайте^3 &&1 ^7чтобы лечь",
            "Hold down^3 &&1 ^7to go prone",
            "^3 &&1 ^7を長押しして伏せる",
            "للإنبطاح اضغط باستمرار ^3 &&1 ^7",
            // Real CP1251 instruction bytes, without relying on UTF-8 or words.
            "\xD3\xE4\xE5\xF0\xE6\xE8\xE2\xE0\xE9\xF2\xE5^3 &&1 ^7\xF7\xF2\xEE\xE1\xFB \xEB\xE5\xF7\xFC"};
        const NativeLocalizeEntry* first_prone{};
        for (const auto& hold : prone_locales) {
            std::string expected(hold);
            expected.replace(expected.find("&&1"), 3, 1, icon_character(ButtonIcon::b));
            for (const auto prone_key : prone_prompt_keys) {
                require(is_hud_prompt_lookup(27, prone_key, hud_localize_return_rva) &&
                            !is_hud_prompt_lookup(26, prone_key, hud_localize_return_rva) &&
                            !is_hud_prompt_lookup(27, prone_key, 0x28A6B0),
                        "All audited prone instructions use only the localization HUD call site");
                const std::string original = "Press ^3[{toggleprone}]^7 to go prone.";
                NativeLocalizeEntry prone{original.c_str(), prone_key.data()};
                const auto value = localized_prone_instruction(prone_key, original, hold);
                const auto* view = cache.lookup(&prone, prone_key, original, value);
                require(value == expected && view != &prone && std::string(view->value) == expected &&
                            prone.value == original.c_str() && std::string(prone.value) == original,
                        "Entire native Hold translation and its encoding reach the HUD with B");
                require(localized_prone_instruction(prone_key, value, hold) == value,
                        "Prone conversion is idempotent for every locale and key");
                require(cache.lookup(&prone, prone_key, original, original) == &prone,
                        "Keyboard/disabled path retains the original entry after controller conversion");
                if (!first_prone) first_prone = view;
                const std::string parameterized = "Press^3 &&1 ^7to go prone";
                require(localized_prone_instruction(prone_key, parameterized, hold) == hold,
                        "Native parameter remains available to consume the caller's button argument");
            }
        }
        require(first_prone && std::string(first_prone->value) == "Удерживайте^3 \x02 ^7чтобы лечь",
                "Locale changes preserve previously returned prone entries");
        for (const auto unrelated : {"WARLORD_HINT_CROUCH", "CGAME_PRONE_BLOCKED", "MENU_PRONE",
                                     "WARLORD_PRONE_DEATH", "SCRIPT_PLATFORM_HINT_STANDFROMPRONEKEY",
                                     "SCRIPT_PLATFORM_HINT_RAISEFROMPRONETOCROUCH", "SUBTITLE_CASTLE_PRI_GETDOWN12"}) {
            require(!is_prone_prompt(unrelated) && !is_hud_prompt_lookup(27, unrelated, hud_localize_return_rva) &&
                        localized_prone_instruction(unrelated, "original", prone_locales[0]) == "original",
                    "Crouch, stand-up, blocked messages, menu labels, death quotes and subtitles are excluded");
        }
        for (const auto invalid : {"", "SCRIPT_PLATFORM_HINT_HOLDDOWNPRONEKEY", "@SCRIPT_PLATFORM_HINT_HOLDDOWNPRONEKEY",
                                   "Hold", "Hold &&2", "Hold &&11", "Hold &&1 and &&1", "Hold &&1 and &&2",
                                   "Hold &&1 and {+attack}", "Hold &&1 \x02"})
            require(localized_prone_instruction("PRAGUE_HINT_PRONE", "original", invalid) == "original",
                    "Missing and malformed native templates keep the original instruction");
        require(is_prone_prompt("@WARLORD_HINT_PRONE") && is_prone_prompt("\x14" "WARLORD_HINT_PRONE") &&
                    localized_prone_instruction("PRAGUE_HINT_PRONE", "", prone_locales[0]).empty() &&
                    localized_prone_instruction("PRAGUE_HINT_PRONE", "Press &&1 and &&2", prone_locales[0]) ==
                        "Press &&1 and &&2",
                "Native key markers are accepted; empty and multi-argument targets stay unchanged");
        require(is_hud_prompt_lookup(27, mortar_prompt_key, hud_localize_return_rva) &&
                    !is_hud_prompt_lookup(27, mortar_prompt_key, 0x28A6B0) &&
                    !is_hud_prompt_lookup(26, mortar_prompt_key, hud_localize_return_rva) &&
                    is_mortar_prompt("@WARLORD_HINT_USE_MORTAR") &&
                    !find_use_hold_prompt(mortar_prompt_key) &&
                    !find_use_hold_rule({0, 0, -1, mortar_prompt_key}),
                "Mortar hint is routed independently of timed Hold actions");
        struct MortarLocale { HoldTextTemplates templates; std::string text, expected; };
        const std::vector<MortarLocale> mortar_locales{
            {{{"Нажмите^3 &&1 ^7, чтобы лечь", "Нажмите^3 &&1 ^7, чтобы пригнуться."},
              {"Нажмите и удерживайте^3 &&1 ^7, чтобы использовать предмет",
               "Нажмите и удерживайте^3 &&1 ^7, чтобы подобрать мины"}},
             "Нажмите и удерживайте ^3&&1^7, чтобы использовать миномет.",
             "Нажмите^3 &&1^7, чтобы использовать миномет."},
            {{{"Press^3 &&1 ^7to go prone", "Press^3 &&1 ^7to crouch."},
              {"Press and hold^3 &&1 ^7to use", "Press and hold ^3&&1^7 to pick up"}},
             "Press and hold ^3&&1^7 to use the mortar.", "Press^3 &&1^7 to use the mortar."},
            {{{"اضغط ^3&&1^7 للإنبطاح", "اضغط ^3&&1^7 للجلوس"},
              {"استمر في الضغط ^3&&1^7 للاستعمال", "استمر في الضغط ^3&&1^7 للالتقاط"}},
             "استمر في الضغط ^3&&1^7 لاستعمال الهاون", "اضغط ^3&&1^7 لاستعمال الهاون"},
            {{{"伏せるには^3&&1^7を押す", "しゃがむには^3&&1^7を押す"},
              {"使うには^3&&1^7を長押しする", "拾うには^3&&1^7を長押しする"}},
             "迫撃砲を使うには^3&&1^7を長押しする", "迫撃砲を使うには^3&&1^7を押す"}};
        const NativeLocalizeEntry* first_mortar{};
        for (const auto& locale : mortar_locales) {
            const auto value = localized_mortar_instruction(mortar_prompt_key, locale.text, locale.templates);
            require(value == locale.expected &&
                        localized_mortar_instruction(mortar_prompt_key, value, locale.templates) == value &&
                        controller_instruction(mortar_prompt_key, value) == value,
                    "Mortar Press preserves the localized action, native argument and repeated conversion");
            NativeLocalizeEntry mortar{locale.text.c_str(), mortar_prompt_key.data()};
            const auto* view = cache.lookup(&mortar, mortar_prompt_key, locale.text, value);
            require(view != &mortar && std::string(view->value) == locale.expected &&
                        mortar.value == locale.text.c_str() &&
                        cache.lookup(&mortar, mortar_prompt_key, locale.text, locale.text) == &mortar,
                    "Mortar HUD uses a private Press view; keyboard/disabled input retains the asset");
            if (!first_mortar) first_mortar = view;
            require(localized_mortar_instruction("PLATFORM_SWAPWEAPONS", locale.text, locale.templates) == locale.text &&
                        localized_mortar_instruction("WARLORD_HINT_PRONE", locale.text, locale.templates) == locale.text &&
                        localized_mortar_instruction("WARLORD_MORTAR_DEATH", locale.text, locale.templates) == locale.text,
                    "Mortar policy cannot change weapon, prone or death text");
        }
        auto missing_mortar = mortar_locales[0].templates;
        missing_mortar.press[0] = "SCRIPT_PLATFORM_HINT_PRONEKEY";
        auto incompatible_mortar = mortar_locales[0].templates;
        incompatible_mortar.hold[1] = "Другая инструкция^3 &&1 ^7";
        require(first_mortar && std::string(first_mortar->value) == mortar_locales[0].expected &&
                    localized_mortar_instruction(mortar_prompt_key, mortar_locales[0].text, missing_mortar) ==
                        mortar_locales[0].text &&
                    localized_mortar_instruction(mortar_prompt_key, mortar_locales[0].text, incompatible_mortar) ==
                        mortar_locales[0].text &&
                    localized_mortar_instruction(mortar_prompt_key, "Hold &&1 and &&2", mortar_locales[1].templates) ==
                        "Hold &&1 and &&2",
                "Missing/incompatible templates and multi-button targets retain the original; returned pointers survive locale changes");
        std::cout << "Native HUD entry routing, locale preservation, input gating and lifetime verified\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
