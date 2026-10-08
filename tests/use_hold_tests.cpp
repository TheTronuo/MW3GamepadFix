#include "mw3gf/game/use_hold.hpp"
#include "mw3gf/core/gameplay_prompts.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
template <class T, std::size_t N> void field(std::array<std::byte, N>& object, std::size_t at, T value) {
    std::memcpy(object.data() + at, &value, sizeof(value));
}
int queried_class = 9, config_index = -1, config_calls = 0;
bool queried_alt = true;
std::string hint_key;
int weapon_class(std::uint32_t id, bool alternate) {
    require(id == 7, "Native classifier receives the selected weapon ID");
    queried_alt = alternate;
    return queried_class;
}
void config_string(int index, char* output, int capacity) {
    config_index = index;
    ++config_calls;
    require(capacity > static_cast<int>(hint_key.size()), "Configstring output is bounded");
    std::memcpy(output, hint_key.c_str(), hint_key.size() + 1);
}
} // namespace
int main() {
    using namespace mw3gf;
    using namespace mw3gf::game;
    using namespace use_layout;
    try {
        require(use_hold_rules().size() == 4, "Four native rules cover the five requested flows");
        UseHold hold;
        std::array<std::byte, 0x118> player{};
        std::array<std::byte, 0xAEF0> state{};
        std::array<std::byte, 0x2A0> target{};
        field(player, client, state.data());
        field(target, entity_type, std::uint8_t{2});
        field(target, weapon, std::uint32_t{7});
        field(target, entity_number, std::uint16_t{42});
        field(state, use_handle, std::uint16_t{43});
        field(state, use_start, std::uint32_t{1000});
        field(state, command, std::uint32_t{1010});
        field(state, buttons, usereload);
        hold.record_command(1010, true);
        const auto rule = [&] { return native_use_hold_rule(target.data(), weapon_class, config_string); };
        const auto blocked = [&](std::uint32_t time, std::uintptr_t caller = timed_entity_use_return_rva,
                                 bool active = true) {
            return hold.defer(player.data(), target.data(), caller, time, active, rule());
        };
        const auto original_player = player;
        const auto original_state = state;
        const auto original_target = target;
        require(rule() && rule()->action == UseAction::ground_weapon &&
                    blocked(1000) && blocked(1249) && !blocked(1250), "Ground weapons require 250 ms");
        require(player == original_player && state == original_state && target == original_target,
                "Deferral never writes native handles, clocks, commands or entities");
        require(!blocked(1100, 0x175D87), "Immediate Use call sites remain native");
        hold.record_command(1020, false);
        require(blocked(1100), "Queued pad command retains its source after physical release");
        field(state, command, std::uint32_t{1020});
        require(!blocked(1100), "Keyboard commands remain immediate with a pad connected");
        field(state, command, std::uint32_t{999});
        require(!blocked(1100), "Unknown command sources remain native");
        field(state, command, std::uint32_t{1010});
        field(state, buttons, std::uint32_t{8});
        require(!blocked(1100), "Keyboard activate bypasses the controller delay");
        field(state, buttons, std::uint32_t{0});
        require(!blocked(1100), "Release and reload do not become held interactions");
        field(state, buttons, usereload);
        require(blocked(1500, timed_entity_use_return_rva, false), "Inactive queued pad Use cancels");
        field(target, entity_type, std::uint8_t{3});
        require(rule() && rule()->action == UseAction::throwing_knife && !queried_alt &&
                    blocked(1249) && !blocked(1250), "Thrown knife uses the shared hold");
        queried_class = 6;
        require(!rule() && !blocked(1100), "Ordinary grenade throwback remains untouched");
        queried_class = 9;
        require(!native_use_hold_rule(target.data(), nullptr, config_string),
                "An unclassified projectile cannot enter the knife rule");
        field(target, entity_type, std::uint8_t{0});
        field(target, cursor_hint, std::uint32_t{1});
        field(target, hint_index, std::uint8_t{3});
        for (const auto& candidate : use_hold_rules()) {
            if (candidate.hint_key.empty()) continue;
            hint_key = candidate.hint_key;
            require(rule() == &candidate && config_index == hint_config_base + 3 &&
                        blocked(1249) && !blocked(1250), "Native hint keys select intel and breach rules");
            field(target, entity_type, std::uint8_t{5});
            require(rule() == &candidate, "The same key works on a usable script entity");
            field(target, entity_type, std::uint8_t{0});
            require(find_use_hold_prompt(candidate.prompt_keys[0]) == &candidate,
                    "Behavior and prompt eligibility come from the same catalog");
        }
        hint_key = "@SCRIPT_INTELLIGENCE_PICKUP";
        require(rule() && rule()->action == UseAction::intelligence, "Localized key prefix is recognized");
        hint_key = "\x14SCRIPT_PLATFORM_BREACH_ACTIVATE";
        require(rule() && rule()->action == UseAction::breach, "Native localized segment marker is recognized");
        hint_key += "\x15plain text";
        require(!rule(), "Composite strings are not misclassified as an audited hint");
        for (const auto* key : {"PLATFORM_HOLD_TO_USE", "NY_MANHATTAN_HINT_ENTER_HIND", "CUSTOM_DOOR"}) {
            hint_key = key;
            require(!rule() && !blocked(1100), "Unaudited doors and other Use hints remain native");
        }
        hint_key = "SCRIPT_INTELLIGENCE_PICKUP";
        for (const auto type : {8u, 9u, 13u, 255u}) {
            field(target, entity_type, static_cast<std::uint8_t>(type));
            require(!rule(), "Turrets, vehicles and NPCs are not added accidentally");
        }
        field(target, entity_type, std::uint8_t{0});
        const auto before_calls = config_calls;
        field(target, hint_index, std::uint8_t{255});
        require(!rule() && config_calls == before_calls, "Missing hint index does not query configstrings");
        field(target, hint_index, std::uint8_t{32});
        require(!rule() && config_calls == before_calls, "Configstring indexes stay inside the native hint table");
        field(target, hint_index, std::uint8_t{3});
        field(target, cursor_hint, std::uint32_t{0});
        require(!rule() && config_calls == before_calls, "Disabled Use hint is not classified");
        field(target, entity_type, std::uint8_t{2});
        field(target, weapon, std::uint32_t{0});
        require(!blocked(1100), "Non-weapon item remains native");
        field(target, weapon, std::uint32_t{7});
        field(state, use_handle, std::uint16_t{44});
        require(!blocked(1100), "Another selected entity cannot borrow the timer");
        field(state, use_handle, std::uint16_t{43});
        field(state, 0x1B4, std::uint32_t{0});
        field(state, 0x1BC, std::uint32_t{0x7FF});
        require(blocked(1249), "Changed player cursor hint cannot bypass a selected item's timer");
        field(state, use_start, std::uint32_t{0xFFFFFF80});
        require(blocked(0x79) && !blocked(0x7A), "Threshold survives clock wrap");
        field(state, use_start, std::uint32_t{2000});
        require(blocked(10), "Clock reset cannot complete an old hold");
        require(!hold.defer(nullptr, target.data(), timed_entity_use_return_rva, 1100, true, rule()) &&
                    !native_use_hold_rule(nullptr, weapon_class, config_string), "Missing native pointers are rejected");
        hold.record_command(1010, false);
        require(!blocked(1100), "Newest tag wins for repeated command times");

        const HoldTextTemplates weapon_text{{"Press^3 &&1 ^7to swap for", "Press^3 &&1 ^7to pick up"},
                                            {"Hold^3 &&1 ^7to plant explosives", "Hold^3 &&1 ^7to defuse explosives"}};
        require(localized_hold_instruction(weapon_text.press[0], weapon_text) == "Hold^3 &&1 ^7to swap for" &&
                    localized_hold_instruction(weapon_text.press[1], weapon_text) == "Hold^3 &&1 ^7to pick up",
                "Weapon pickup, swap and knife retain their action text");
        const HoldTextTemplates interaction_text{
            {"Press ^3[{+activate}]^7 to secure the enemy intelligence.", "Press ^3 [{+activate}] ^7 to breach"},
            weapon_text.hold};
        for (const auto key : {"SCRIPT_INTELLIGENCE_PICKUP", "SCRIPT_PLATFORM_BREACH_ACTIVATE"}) {
            const auto* prompt = find_use_hold_prompt(key);
            require(prompt && prompt->press_reference_keys[0] == "SCRIPT_INTELLIGENCE_PICKUP",
                    "Script prompts carry their own reference pair");
        }
        const auto intel = localized_hold_instruction(interaction_text.press[0], interaction_text);
        const auto breach = localized_hold_instruction(interaction_text.press[1], interaction_text);
        require(intel == "Hold^3 [{+activate}]^7 to secure the enemy intelligence." &&
                    breach == "Hold^3 [{+activate}] ^7 to breach", "Native formatting differences preserve actions and slots");
        require(controller_instruction("SCRIPT_INTELLIGENCE_PICKUP", intel) ==
                    "Hold^3 \x03^7 to secure the enemy intelligence.", "Localized hold composes with the button icon");
        require(localized_hold_instruction(intel, interaction_text) == intel,
                "Already converted Hold instruction is idempotent");
        const HoldTextTemplates russian{{"Нажмите^3 &&1 ^7для подбора", "Нажмите ^3[{+activate}]^7для штурма"},
                                        {"Удерживайте^3 &&1 ^7для установки", "Удерживайте^3 &&1 ^7для снятия"}};
        require(localized_hold_instruction(russian.press[0], russian) == "Удерживайте^3 &&1 ^7для подбора",
                "Russian instruction comes from native templates");
        const HoldTextTemplates russian_retail{
            {"^3 &&1 ^7- обменять на", "Нажмите^3 &&1 ^7, чтобы взять"},
            {"Нажать и удерживать ^3 &&1 ^7- установить взрывчатку",
             "Нажать и удерживать ^3 &&1 ^7- обезвредить взрывчатку"},
            true};
        require(find_use_hold_prompt("PLATFORM_SWAPWEAPONS")->allow_implicit_prefix &&
                    find_use_hold_prompt("PLATFORM_PICKUPNEWWEAPON")->allow_implicit_prefix &&
                    !find_use_hold_prompt("SCRIPT_INTELLIGENCE_PICKUP")->allow_implicit_prefix,
                "Implicit instruction support belongs only to the audited weapon rules");
        const auto russian_swap = localized_hold_instruction(russian_retail.press[0], russian_retail);
        const auto russian_pickup = localized_hold_instruction(russian_retail.press[1], russian_retail);
        require(russian_swap == "Нажать и удерживать ^3 &&1 ^7- обменять на" &&
                    russian_pickup == "Нажать и удерживать ^3 &&1 ^7, чтобы взять",
                "Actual Russian retail weapon strings gain Hold and preserve each action");
        require(controller_instruction("PLATFORM_SWAPWEAPONS", russian_swap) ==
                    "Нажать и удерживать ^3 &&1 ^7- обменять на" &&
                    localized_hold_instruction(russian_swap, russian_retail) == russian_swap &&
                    localized_hold_instruction(russian_pickup, russian_retail) == russian_pickup,
                "Russian retail Hold preserves the engine's button parameter and is idempotent");
        auto implicit_disabled = russian_retail;
        implicit_disabled.allow_implicit_prefix = false;
        require(localized_hold_instruction(russian_retail.press[0], implicit_disabled) == russian_retail.press[0] &&
                    localized_hold_instruction("Своя команда^3 &&1 ^7- обменять на", russian_retail) ==
                        "Своя команда^3 &&1 ^7- обменять на",
                "Implicit Hold neither escapes its policy nor replaces arbitrary prefixes");
        const HoldTextTemplates russian_script{
            {"Нажмите ^3[{+activate}]^7, чтобы забрать разведданные.",
             "Нажмите ^3[{+activate}]^7, чтобы штурмовать"}, russian_retail.hold};
        require(localized_hold_instruction(russian_script.press[0], russian_script) ==
                    "Нажать и удерживать ^3 [{+activate}]^7, чтобы забрать разведданные." &&
                    localized_hold_instruction(russian_script.press[1], russian_script) ==
                    "Нажать и удерживать ^3 [{+activate}]^7, чтобы штурмовать",
                "Actual Russian script strings retain their original actions");
        const HoldTextTemplates arabic{{"اضغط ^3&&1^7 لالتقاط", "اضغط ^3[{+activate}]^7 للدخول"},
                                       {"استمر في الضغط ^3&&1^7 لزرع", "استمر في الضغط ^3&&1^7 لنزع"}};
        require(localized_hold_instruction(arabic.press[1], arabic) ==
                    "استمر في الضغط ^3[{+activate}]^7 للدخول", "Arabic needs no English word matching");
        const HoldTextTemplates japanese{{"交換するには^3&&1^7を押す", "突入するには^3[{+activate}]^7を押す"},
                                         {"設置するには^3&&1^7を長押しする", "解除するには^3&&1^7を長押しする"}};
        require(localized_hold_instruction(japanese.press[1], japanese) ==
                    "突入するには^3[{+activate}]^7を長押しする", "Post-button instruction keeps its action prefix");
        auto japanese_weapon = japanese;
        japanese_weapon.allow_implicit_prefix = true;
        require(localized_hold_instruction(japanese.press[1], japanese_weapon) ==
                    "突入するには^3[{+activate}]^7を長押しする",
                "Implicit prefix permission leaves suffix-instruction languages intact");
        require(localized_hold_instruction("custom", weapon_text) == "custom" &&
                    localized_hold_instruction("Press &&1 and &&1", weapon_text) == "Press &&1 and &&1" &&
                    localized_hold_instruction("Press &&1 and {+attack}", weapon_text) == "Press &&1 and {+attack}" &&
                    localized_hold_instruction("Press &&1 to pick up", {{"different &&1", "Press &&1"}, weapon_text.hold}) ==
                    "Press &&1 to pick up", "Missing, ambiguous or incompatible templates remain intact");
        require(!find_use_hold_prompt("PLATFORM_RELOAD") && !find_use_hold_prompt(""),
                "Unrelated prompt keys remain untouched");
        std::cout << "Five Use flows: native eligibility, command sources, timing and localized text verified\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
