#include "mw3gf/core/gameplay_adapter.hpp"
#include "mw3gf/game/remote_control.hpp"
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace {
void require(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
// Native RemoteControlMove writes pitch at +3E, yaw at +3F on both platforms.
void check(mw3gf::Stick move, mw3gf::Stick look, bool inverted, int pitch, int yaw) {
    std::array<std::int8_t, 64> command;
    command.fill(42);
    command[0x3E] = command[0x3F] = 0;
    mw3gf::game::add_remote_control(command.data(), move, look, inverted);
    require(command[0x3E] == pitch && command[0x3F] == yaw, "Xbox remote axis order and signs");
    for (std::size_t i = 0; i < 0x3E; ++i)
        require(command[i] == 42, "Remote steering does not overwrite ordinary camera or buttons");
}
} // namespace
int main() {
    try {
        using namespace mw3gf;
        check({}, {0, 1}, false, -127, 0); // Up changes only pitch.
        check({}, {0, -1}, false, 127, 0);
        check({}, {1, 0}, false, 0, -127); // Right changes only yaw.
        check({}, {-1, 0}, false, 0, 127);
        check({}, {0, 1}, true, 127, 0);
        check({}, {0, -1}, true, -127, 0);
        check({}, {1, 0}, true, 0, -127); // Inversion never changes yaw.
        check({}, {.5f, -.5f}, false, 64, -63); // Native floor(v*127+.5) rounding.
        check({}, {}, false, 0, 0);
        check({0, 1}, {}, false, -127, 0);
        check({0, -1}, {}, false, 127, 0);
        check({1, 0}, {}, false, 0, -127);
        check({-1, 0}, {}, false, 0, 127);
        check({0, .5f}, {}, false, -32, 0); // LS MAP_SQUARED.
        check({.5f, .5f}, {}, false, -45, -45); // Pair radius, no walking diagonal correction.
        check({.5f, 0}, {-.25f, 0}, false, 0, 0); // Opposite sticks cancel before packing.
        check({0, 1}, {0, 1}, false, -127, 0); // Combined input saturates.
        check({0, .5f}, {}, true, 32, 0);
        const float nan = std::numeric_limits<float>::quiet_NaN();
        check({}, {nan, 1}, false, 0, 0);
        check({nan, 0}, {1, 0}, false, 0, -127);
        game::add_remote_control(nullptr, {}, {}, false);

        std::array<std::int8_t, 64> command{};
        command[0x3E] = 100;
        command[0x3F] = -100;
        game::add_remote_control(command.data(), {}, {1, -1}, false);
        require(command[0x3E] == 127 && command[0x3F] == -127, "Native mouse input merges without overflow");
        command[0x3E] = 64;
        command[0x3F] = -63;
        game::add_remote_control(command.data(), {}, {-.5f, .5f}, false);
        require(command[0x3E] == 1 && command[0x3F] == 1, "Native mouse and controller can oppose each other");

        GameplayAdapter adapter;
        InputSnapshot pad;
        pad.connected = true;
        pad.device_id = L"test";
        pad.backend = L"xinput";
        pad.pad.left_stick = pad.pad.right_stick = {0, 1};
        auto output = adapter.update(pad, true);
        require(!output.move.y && !output.look.y, "Held sticks remain blocked when gameplay starts");
        pad.pad = {};
        (void)adapter.update(pad, true);
        pad.pad.right_stick = {0, 1};
        output = adapter.update(pad, true);
        check(output.move, output.look, false, -127, 0);
        pad.pad = {};
        pad.pad.left_stick = {1, 0};
        output = adapter.update(pad, true);
        check(output.move, output.look, false, 0, -127);
        pad.connected = false;
        output = adapter.update(pad, true);
        check(output.move, output.look, false, 0, 0);
        pad.connected = true;
        output = adapter.update(pad, false);
        check(output.move, output.look, false, 0, 0);
        std::cout << "Xbox remote steering directions, both sticks, inversion, packing and input gating verified\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
