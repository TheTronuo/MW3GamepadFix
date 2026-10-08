#include "mw3gf/core/cinematic_skip.hpp"
#include "mw3gf/core/gameplay_adapter.hpp"
#include "mw3gf/core/menu_adapter.hpp"
#include <iostream>
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
        CinematicSkip skip;
        InputSnapshot input;
        input.connected = true;
        input.device_id = L"controller1";
        input.backend = L"test";
        input.pad.buttons = bit(Button::a);
        require(!skip.update(input, true), "Held A on startup does not skip");
        require(!skip.update(input, true), "Held A cannot repeat");
        input.pad.buttons = 0;
        require(!skip.update(input, true), "Release does not skip");
        input.pad.buttons = bit(Button::a);
        require(skip.update(input, true), "Fresh A press skips");
        require(!skip.update(input, true), "One press skips only one video");
        require(!skip.update(input, false), "A cannot skip in menus or gameplay");
        require(!skip.update(input, true), "Held A when playback begins is suppressed");
        input.pad.buttons = 0;
        (void)skip.update(input, true);
        input.pad.buttons = bit(Button::b) | bit(Button::start) | bit(Button::x);
        require(!skip.update(input, true), "Other controls cannot skip");
        input.pad.buttons |= bit(Button::a);
        require(skip.update(input, true), "A works alongside other buttons");
        input.connected = false;
        require(!skip.update(input, true), "Disconnect releases stale input");
        input.connected = true;
        require(!skip.update(input, true), "Reconnect with held A cannot skip");
        input.device_id = L"controller2";
        require(!skip.update(input, true), "Switching device suppresses held A");
        input.backend = L"other backend";
        require(!skip.update(input, true), "Switching backend suppresses held A");
        input.pad.buttons = 0;
        (void)skip.update(input, true);
        input.pad.buttons = bit(Button::a);
        require(skip.update(input, true), "New device can skip after release");

        MenuAdapter menu;
        (void)menu.update(input, {false, 0, 0}, 0);
        require(menu.update(input, {true, 123, 0}, 1).count == 0, "Skip cannot confirm the first menu");
        require(menu.update(input, {true, 123, 0}, 2).count == 0, "Held skip stays blocked in the menu");
        input.pad.buttons = 0;
        (void)menu.update(input, {true, 123, 0}, 3);
        input.pad.buttons = bit(Button::a);
        const auto confirm = menu.update(input, {true, 123, 0}, 4);
        require(confirm.count == 1 && confirm.values[0] == MenuAction::confirm,
                "A confirms normally after being released");

        GameplayAdapter gameplay;
        (void)gameplay.update(input, false);
        require(gameplay.update(input, true).count == 0, "Skip cannot become a jump when gameplay starts");
        std::cout << "Cinematic skip transitions verified\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
