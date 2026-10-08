#include "mw3gf/core/gameplay_adapter.hpp"
#include "mw3gf/core/gameplay_prompts.hpp"
#include "mw3gf/core/prompt_catalog.hpp"
#include <iostream>
#include <stdexcept>
namespace {
void require(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
} // namespace
int main() {
    try {
        using namespace mw3gf;
        InputSnapshot pad;
        pad.connected = true;
        pad.device_id = L"1";
        pad.backend = L"xinput";
        GameplayAdapter adapter;
        require(adapter.update(pad, true).count == 0, "Neutral entry");
        for (std::size_t i = 0; i < gameplay_bindings.size(); ++i) {
            const auto& b = gameplay_bindings[i];
            pad.pad = {};
            if (b.control == control_lt)
                pad.pad.left_trigger = 1;
            else if (b.control == control_rt)
                pad.pad.right_trigger = 1;
            else
                pad.pad.buttons = b.control;
            const auto down = adapter.update(pad, true);
            require(down.count == 1 && down.events[0].binding == i && down.events[0].down,
                    "Every input gets its own down");
            require(adapter.update(pad, true).count == 0, "Holds don't repeat discrete actions");
            pad.pad = {};
            const auto up = adapter.update(pad, true);
            require(up.count == (b.up ? 1u : 0u), "Paired native release");
            require(gameplay_glyph(b.command) == b.glyph, "Prompt follows real binding");
        }
        pad.pad.right_trigger = 1;
        (void)adapter.update(pad, true);
        auto release = adapter.update(pad, false);
        require(release.count == 1 && !release.events[0].down, "Menu entry releases fire");
        require(adapter.update(pad, true).count == 0, "Held fire cannot leak out of menu");
        pad.pad = {};
        (void)adapter.update(pad, true);
        pad.pad.left_stick = {0, 1};
        auto move = adapter.update(pad, true);
        require(move.forward == 127 && move.right == 0, "Forward analog movement");
        pad.pad.left_stick = {1, -1};
        move = adapter.update(pad, true);
        require(move.forward <= -126 && move.right >= 126,
                "Xbox diagonal compensation retains full movement on both axes");
        pad.pad.left_stick = {0.1f, 0.1f};
        move = adapter.update(pad, true);
        require(!move.forward && !move.right, "Radial deadzone");
        pad.pad.right_stick = {1, 1};
        require(adapter.update(pad, true).count == 0,
                "Right stick uses analog camera input, not discrete commands");
        pad.pad.buttons = bit(Button::x);
        (void)adapter.update(pad, true);
        pad.connected = false;
        require(adapter.update(pad, true).count == 1, "Disconnect releases use/reload");
        pad.connected = true;
        pad.device_id = L"2";
        require(adapter.update(pad, true).count == 0, "Reconnect blocks held buttons");
        require(merge_movement(100, 90) == 127 && merge_movement(100, -100) == 0,
                "Movement preserves keyboard contribution without overflow");
        require(expand_action_prompts("Hold [{+activate}] to use") == "Hold \x03 to use",
                "Action token uses X");
        require(expand_action_prompts("^3[Left Mouse]^7 Fire") == "^3[Left Mouse]^7 Fire",
                "No language-specific literal substitution");
        require(controller_instruction("PLATFORM_USE_BUTTONLOOK_TO_AIM", "Hold ^3[Right Mouse]^7 to aim") ==
                    "\x12",
                "Context prompt follows working LT input");
        require(controller_instruction("ordinary_text", "Left Mouse F1") == "Left Mouse F1",
                "Ordinary text is preserved");
        require(controller_instruction("PLATFORM_HINT_MOVEONTRUCK",
                                       "Press movement keys to move on truck.") == "\x10",
                "Movement context uses left stick without reading translated words");
        require(controller_instruction("PLATFORM_USE_BUTTONLOOK_TO_AIM", "Hold \xCF\xCA\xCC to aim") ==
                    "\x12",
                "Legacy Russian hint uses the same semantic key");
        require(expand_action_prompts("^3[{weapnext}]^7 / [{+lookup}] / [{chatmodepublic}]") ==
                    "^3\x04^7 / \x11 / [{chatmodepublic}]",
                "Formatting, reserved camera prompt and unmapped actions survive substitution");
        require(gameplay_glyph("+lookup") == 17u, "Camera action uses the working right stick");
        require(action_prompt_glyph("+lookup") == 17u, "Camera prompt matches input");
        const std::string material = std::string("^3^") + char(1) + char(16) + char(17) + char(3) + "gun";
        require(!contains_prompt_glyph(material),
                "Weapon record mode/dimensions never count as a controller button");
        require(expand_action_prompts(material) == material,
                "Action resolver preserves native weapon material");
        require(controller_instruction("PLATFORM_USE_BUTTONLOOK_TO_AIM", "Beliebige Sprache") == "\x12",
                "Context doesn't depend on English/Russian input names");
        require(gameplay_glyph("+reload") == 3u && gameplay_glyph("+activate") == 3u,
                "PC split use and reload share X");
        require(controller_instruction("PLATFORM_RELOAD", "Reload") == "\x03 Reload",
                "Low-ammo reload warning includes X");
        require(controller_instruction("@PLATFORM_RELOAD", "^3إعادة التلقيم^7") == "\x03 ^3إعادة التلقيم^7",
                "Reload preserves the complete translated label and its colors");
        require(controller_instruction("PLATFORM_LOW_AMMO_NO_RELOAD", "Low Ammo") == "Low Ammo",
                "No reload button is advertised when reloading is unavailable");
        require(controller_instruction("PLATFORM_RELOAD", material) == "\x03 " + material,
                "Binary material fields cannot suppress a missing fixed prompt");
        for (const auto& rule : detail::fixed_prompts) {
            for (const std::string label : {"Arbitrary label", "Перевод", "نص مترجم"}) {
                const auto result = controller_instruction(rule.key, label);
                require(contains_prompt_glyph(result), "Every audited fixed context produces a button");
                if (rule.kind != detail::FixedPromptKind::icon_only)
                    require(result.ends_with(label), "Unmarked localized label bytes are preserved");
                require(controller_instruction(rule.key, result) == result,
                        "Fixed prompts never duplicate buttons on repeated localization");
            }
        }
        require(controller_instruction("PLATFORM_VEH_FIRE", "[زر] إطلاق") == "\x13 إطلاق",
                "Marked input fields use working controls without matching translated key names");
        require(controller_instruction("PLATFORM_BACK_CAPS", "Назад ^2ESC^7") == "Назад ^2\x02^7",
                "Keyboard shortcut is replaced in place, preserving the translated action");
        require(controller_instruction("PLATFORM_RELOAD", "Unrelated [label]") == "\x03 Unrelated [label]",
                "Plain captions are not parsed as keyboard fields");
        const auto eog =
            controller_instruction("PLATFORM_EOG_PRESS_ESC", "Press^3 ESC ^7to^2 Create a Class ^7and more");
        require(eog.find("ESC") == std::string::npos && eog.find("Create a Class") != std::string::npos &&
                    controller_instruction("PLATFORM_EOG_PRESS_ESC", eog) == eog,
                "Only the input field changes; highlighted action labels remain intact");
        require(controller_instruction("PLATFORM_UI_SELECTBUTTON_ENABLE", "اضغط ^3&&1^7 للتفعيل") ==
                    "اضغط ^3\x01^7 للتفعيل",
                "Menu selection substitutes the semantic button parameter in any language");
        const auto sdv = controller_instruction("NY_HARBOR_PLATFORM_HINT_DRIVE_SDV_3",
                                                "Push ^3[{+forward}]^7 to accelerate and ^3mouse^7 to steer");
        require(sdv.find('\x10') != std::string::npos && sdv.find('\x11') != std::string::npos &&
                    sdv.find("mouse") == std::string::npos,
                "Submarine steering includes both movement and camera controls");
        require(controller_instruction("SCRIPT_PLATFORM_STEER_DRONE", "نص مترجم") == "\x11",
                "Unmarked mouse-only missile guidance uses a language-independent stick prompt");
        require(controller_instruction("CASTLE_HINT_OPEN_DOOR", "Press [{+activate}] to open") ==
                    "Press \x03 to open",
                "Mission-specific localization keys expand actions before formatting");
        const std::array<std::pair<std::string_view, unsigned>, 30> campaign_actions{{
            {"+actionslot 1", 20}, {"+actionslot 2", 21},
            {"+actionslot 3", 22}, {"+actionslot 4", 23},
            {"+activate", 3},      {"+attack", 19},
            {"+back", 16},         {"+changezoom", 17},
            {"+forward", 16},      {"+frag", 6},
            {"+gostand", 1},       {"+holdbreath", 16},
            {"+melee", 17},        {"+movedown", 2},
            {"+prone", 2},         {"+smoke", 5},
            {"+speed", 18},        {"+speed_throw", 18},
            {"+sprint", 16},       {"+stance", 2},
            {"+throw", 19},        {"+toggleads_throw", 18},
            {"+usereload", 3},     {"gocrouch", 2},
            {"goprone", 2},        {"skip", 14},
            {"toggleads", 18},     {"togglecrouch", 2},
            {"toggleprone", 2},    {"weapnext", 4},
        }};
        for (const auto& [action, expected] : campaign_actions)
            require(action_prompt_glyph(action) == expected,
                    "Every action token found in the 93 inspected SP archives has a prompt");
        pad.pad = {};
        pad.pad.buttons = bit(Button::back);
        require(adapter.update(pad, true).count == 0, "Back/View has no gameplay action");
        pad.pad = {};
        (void)adapter.update(pad, true);
        pad.pad.left_stick = {0, 1};
        pad.pad.right_stick = {1, 0};
        auto both = adapter.update(pad, true);
        require(both.forward == 127 && both.look.x == 1, "Both sticks act independently");
        require(adapter.update(pad, false).reset_camera, "Pause resets velocity");
        both = adapter.update(pad, true);
        require(!both.forward && !both.look.x, "Held sticks cannot leak out of pause");
        pad.pad.left_stick = {};
        both = adapter.update(pad, true);
        require(!both.look.x, "Neutral movement does not unblock held camera");
        pad.pad.left_stick = {0, 1};
        pad.pad.right_stick = {};
        both = adapter.update(pad, true);
        require(both.forward == 127 && !both.look.x, "Camera and movement neutral gates are independent");
        pad.pad.right_stick = {1, 0};
        both = adapter.update(pad, true);
        require(both.look.x == 1, "Camera resumes after its own neutral");
        std::cout << "Gameplay transitions, movement and prompt mappings verified\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
