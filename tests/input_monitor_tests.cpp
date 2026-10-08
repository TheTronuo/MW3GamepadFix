#include "mw3gf/core/input_monitor.hpp"
#include "mw3gf/core/status_text.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* explanation) {
    if (!condition)
        throw std::runtime_error(explanation);
}
bool contains(const mw3gf::InputMonitor& monitor, std::wstring_view text) {
    for (const auto& event : monitor.events())
        if (event.text == text)
            return true;
    return false;
}
} // namespace
int main() {
    try {
        using namespace mw3gf;
        require(normalize_axis(-32768) == -1.0f && normalize_axis(32767) == 1.0f && normalize_axis(0) == 0.0f,
                "Axis endpoints");
        require(normalize_axis(-1) < 0.0f && normalize_axis(1) > 0.0f, "Small axis values survive");
        require(normalize_trigger(0) == 0.0f && normalize_trigger(255) == 1.0f, "Trigger endpoints");
        InputMonitor monitor;
        InputSnapshot state;
        state.connected = true;
        state.backend = L"fake";
        state.device_id = L"pad1";
        state.device_name = L"Test";
        state.pad.buttons = bit(Button::a);
        monitor.update(state);
        require(contains(monitor, L"A  pressed"), "First press detected");
        const auto count = monitor.events().size();
        monitor.update(state);
        require(monitor.events().size() == count, "Holding a button produces no repeated press edges");
        state.connected = false; // Deliberately leave stale button data supplied by a broken provider.
        monitor.update(state);
        require(!monitor.snapshot().pad.held(Button::a), "Disconnect clears stale buttons");
        require(contains(monitor, L"A  released"), "Disconnect releases held buttons");
        state.connected = true;
        monitor.update(state);
        state.backend = L"second";
        state.device_id = L"pad2";
        monitor.update(state);
        require(contains(monitor, L"A  released") && contains(monitor, L"A  pressed"),
                "Device/backend change resets edge state");
        state.pad.buttons |= bit(Button::rb);
        state.pad.left_trigger = 0.5f;
        state.pad.left_stick.y = 0.75f;
        require(pressed_controls(state) == L"A + RB + LT + LS_UP",
                "Display combines digital and analog controls");
        state.pad = {};
        require(pressed_controls(state) == L"none", "Idle state");
        state.connected = false;
        require(pressed_controls(state) == L"no controller", "Disconnected status");
        std::cout << "Input state, transitions, disconnect/reconnect and display checks passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
