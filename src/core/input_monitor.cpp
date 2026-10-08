#include "mw3gf/core/input_monitor.hpp"
#include <utility>

namespace mw3gf {
void InputMonitor::append(std::wstring text) {
    events_.push_front({std::move(text)});
    constexpr std::size_t max_events = 6;
    if (events_.size() > max_events)
        events_.pop_back();
}
void InputMonitor::edges(std::uint32_t before, std::uint32_t after) {
    for (const auto& [button, label] : button_labels) {
        if (((before ^ after) & bit(button)) != 0)
            append(std::wstring(label) + ((after & bit(button)) ? L"  pressed" : L"  released"));
    }
}
void InputMonitor::update(InputSnapshot next) {
    if (!next.connected)
        next.pad = {}; // Never retain held buttons after disconnect.
    const bool changed_device = snapshot_.device_id != next.device_id || snapshot_.backend != next.backend;
    if (initialized_ && snapshot_.connected && (!next.connected || changed_device)) {
        edges(snapshot_.pad.buttons, 0);
        append(L"Controller disconnected / selection changed");
    }
    const bool continuing = initialized_ && snapshot_.connected && next.connected && !changed_device;
    if (next.connected) {
        if (!continuing)
            append(L"Controller connected: " + next.device_name);
        edges(continuing ? snapshot_.pad.buttons : 0, next.pad.buttons);
    }
    snapshot_ = std::move(next);
    initialized_ = true;
}
} // namespace mw3gf
