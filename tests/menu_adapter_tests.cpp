#include "mw3gf/core/menu_adapter.hpp"
#include "mw3gf/core/menu_footer.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
bool exactly(const mw3gf::MenuActions& actions, mw3gf::MenuAction action) {
    return actions.count == 1 && actions.values[0] == action;
}
} // namespace
int main() {
    try {
        using namespace mw3gf;
        InputSnapshot input;
        input.connected = true;
        input.device_id = L"pad1";
        input.backend = L"test";
        MenuContext context{true, 1};
        MenuAdapter adapter;
        require(adapter.update(input, context, 0).count == 0, "Neutral connection");
        input.pad.buttons = bit(Button::a);
        require(exactly(adapter.update(input, context, 1), MenuAction::confirm), "A confirms");
        require(adapter.update(input, context, 1000).count == 0, "Confirm never repeats");
        input.pad.buttons = 0;
        (void)adapter.update(input, context, 1001);
        input.pad.buttons = bit(Button::b);
        require(exactly(adapter.update(input, context, 1002), MenuAction::back), "B backs out");
        input.pad.buttons = 0;
        (void)adapter.update(input, context, 1003);
        input.pad.buttons = bit(Button::a) | bit(Button::b);
        require(exactly(adapter.update(input, context, 1004), MenuAction::back),
                "Back beats simultaneous confirm");
        input.pad.buttons = bit(Button::down);
        require(exactly(adapter.update(input, context, 2000), MenuAction::down),
                "Direction starts immediately");
        require(adapter.update(input, context, 2419).count == 0, "Initial repeat delay");
        require(exactly(adapter.update(input, context, 2420), MenuAction::down), "Repeat at 420ms");
        require(adapter.update(input, context, 2629).count == 0, "Subsequent repeat delay");
        require(exactly(adapter.update(input, context, 2630), MenuAction::down), "Repeat at 210ms");
        require(exactly(adapter.update(input, context, 9000), MenuAction::down),
                "Long frame emits one repeat");
        require(adapter.update(input, context, 9001).count == 0, "No queued repeat burst");
        input.pad.buttons = bit(Button::up);
        require(exactly(adapter.update(input, context, 9002), MenuAction::up),
                "Direction change is immediate");
        input.pad.buttons = bit(Button::up) | bit(Button::down);
        require(adapter.update(input, context, 9003).count == 0, "Opposite D-pad directions cancel");
        input.pad.buttons = 0;
        input.pad.left_stick.y = 0.49f;
        require(adapter.update(input, context, 10000).count == 0, "Stick entry threshold");
        input.pad.left_stick.y = 0.5f;
        require(exactly(adapter.update(input, context, 10001), MenuAction::up), "Stick positive Y is up");
        input.pad.left_stick.y = 0.31f;
        require(exactly(adapter.update(input, context, 10421), MenuAction::up),
                "Stick hysteresis holds direction");
        input.pad.left_stick.y = 0.3f;
        require(adapter.update(input, context, 11000).count == 0, "Stick release threshold");
        input.pad.left_stick = {0.8f, 0.6f};
        require(exactly(adapter.update(input, context, 11001), MenuAction::right),
                "Analog diagonal uses dominant axis");
        input.pad.buttons = bit(Button::down);
        require(exactly(adapter.update(input, context, 11002), MenuAction::down),
                "D-pad overrides the stick");
        context.identity = 2;
        require(adapter.update(input, context, 11003).count == 0, "New menu suppresses held input");
        require(adapter.update(input, context, 12000).count == 0,
                "Held direction cannot spill into another menu");
        input.pad = {};
        (void)adapter.update(input, context, 12001);
        input.pad.buttons = bit(Button::start);
        require(exactly(adapter.update(input, context, 12002), MenuAction::confirm), "Start confirms in UI");
        context.active = false;
        (void)adapter.update(input, context, 12003);
        context.active = true;
        require(adapter.update(input, context, 12004).count == 0, "Focus return suppresses held Start");
        input.connected = false;
        require(adapter.update(input, context, 12005).count == 0, "Disconnected stale buttons ignored");
        input.connected = true;
        require(adapter.update(input, context, 12006).count == 0, "Reconnect suppresses held button");
        input.pad = {};
        (void)adapter.update(input, context, 12007);
        input.pad.buttons = bit(Button::a);
        input.device_id = L"pad2";
        require(adapter.update(input, context, 12008).count == 0, "Device swap suppresses held button");
        input.pad = {};
        (void)adapter.update(input, context, 12009);
        input.pad.buttons = bit(Button::x) | bit(Button::rb);
        input.pad.right_stick = {1, 1};
        input.pad.left_trigger = 1;
        input.pad.right_trigger = 1;
        require(adapter.update(input, context, 12010).count == 0,
                "Gameplay controls produce no menu commands");
        input.pad.left_stick.x = std::numeric_limits<float>::quiet_NaN();
        require(adapter.update(input, context, 12011).count == 0, "Malformed axis does not navigate");
        require(footer_label("Friends ^2F^7") == "Friends", "Remove F, retain Friends");
        require(footer_label("Leaderboards ^2Right Mouse^7/^2F1^7") == "Leaderboards",
                "Remove mouse and keyboard suffixes");
        require(footer_label("Quit") == "Quit" && footer_label("Exit") == "Exit",
                "Keep original exit wording");
        require(footer_label("\xc4\xf0\xf3\xe7\xfc\xff ^2F^7") == "\xc4\xf0\xf3\xe7\xfc\xff",
                "Keep localized game bytes");
        require(!footer_binding("@MENU_SP_CAMPAIGN"), "Do not replace normal menu content");
        require(footer_binding("@MENU_QUIT")->action == MenuAction::back, "Quit binds B");
        require(footer_binding("@PLATFORM_FRIENDS_SHORTCUT")->action == MenuAction::auxiliary_y,
                "Friends binds Y");
        require(footer_binding("@PLATFORM_GAMESUMMARY_SHORTCUT")->glyph[0] == 15,
                "Xbox BUTTON_BACK glyph, not Start");
        require(footer_binding("@PLATFORM_TOP_SHORTCUT")->glyph[0] == 16, "Xbox BUTTON_LSTICK glyph, not LT");
        require(footer_binding("@PLATFORM_BOTTOM_SHORTCUT")->glyph[0] == 17,
                "Xbox BUTTON_RSTICK glyph, not RT");
        input.pad = {};
        (void)adapter.update(input, context, 13000);
        constexpr std::array shortcut_buttons{Button::y,  Button::x,  Button::back, Button::lb,
                                              Button::rb, Button::ls, Button::rs};
        constexpr std::array shortcut_actions{
            MenuAction::auxiliary_y, MenuAction::auxiliary_x, MenuAction::auxiliary_view, MenuAction::page_up,
            MenuAction::page_down,   MenuAction::top,         MenuAction::bottom};
        std::uint64_t time = 13001;
        for (std::size_t i = 0; i < shortcut_buttons.size(); ++i) {
            context.shortcut_buttons = 0;
            input.pad.buttons = bit(shortcut_buttons[i]);
            require(adapter.update(input, context, time++).count == 0, "Invisible shortcut never fires");
            context.shortcut_buttons = bit(shortcut_buttons[i]);
            require(adapter.update(input, context, time++).count == 0,
                    "Newly shown shortcut does not fire while held");
            input.pad.buttons = 0;
            (void)adapter.update(input, context, time++);
            input.pad.buttons = bit(shortcut_buttons[i]);
            require(exactly(adapter.update(input, context, time++), shortcut_actions[i]),
                    "Visible shortcut fires once");
            require(adapter.update(input, context, time + 1000).count == 0, "Shortcuts never repeat");
            time += 1001;
            context.identity++;
            require(adapter.update(input, context, time++).count == 0,
                    "Held shortcut cannot enter another menu");
            input.pad.buttons = 0;
            (void)adapter.update(input, context, time++);
        }
        std::cout << "Menu actions, Xbox repeat timing, hysteresis, context/focus gating and device "
                     "lifecycle passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
