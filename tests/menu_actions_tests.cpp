#include "mw3gf/game/menu_actions.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}

struct KeyEvent {
    int client;
    int key;
    int down;
    bool operator==(const KeyEvent&) const = default;
};

struct FakeUi {
    int menu{};
    int parent{};
    int dialog{};
    void* active = &menu;
    void* next{};
    int capture = 0x10;
    bool paused = true;
    int cleared_keys{};
    int scripts{};
    void* script_context{};
    void* script_item{};
    void* script_pointer{};
    bool transition_on_key{};
    std::vector<KeyEvent> events;
}* ui;

void* active_menu(void* context) {
    require(context == ui, "Active menu receives native context");
    return ui->active;
}

void key_event(int client, int key, int down) {
    ui->events.push_back({client, key, down});
    if (down && ui->active && ui->transition_on_key)
        ui->active = ui->next;
    // Model the verified UI_KeyEvent no-menu branch in the supported executable.
    if (!ui->active) {
        ui->capture &= ~0x10;
        ui->paused = false;
        ++ui->cleared_keys;
    }
}

void run_script(void* context, void* item, void* script) {
    ++ui->scripts;
    ui->script_context = context;
    ui->script_item = item;
    ui->script_pointer = script;
    // Footer scripts close the menu without the outer UI_KeyEvent cleanup.
    ui->active = ui->next;
}

mw3gf::game::NativeMenuActions actions(FakeUi& state) {
    ui = &state;
    return {&state, active_menu, key_event, run_script};
}

void footer_closes_last_menu() {
    FakeUi state;
    const auto dispatch = actions(state);
    int item{}, script{};
    // Demonstrate the original failure: the raw script leaves UI ownership and pause.
    run_script(&state, &item, &script);
    require(!state.active && state.capture == 0x10 && state.paused,
            "Raw footer close reproduces stuck UI ownership and pause");
    state.active = &state.menu;
    state.scripts = 0;
    state.capture |= 0x2;
    dispatch.run_script(&item, &script);
    require(state.scripts == 1 && state.script_context == &state && state.script_item == &item &&
                state.script_pointer == &script,
            "Native footer callback is preserved exactly once");
    require(!state.active && state.capture == 0x2 && !state.paused && state.cleared_keys == 1,
            "Last menu close releases UI ownership, clears keys and resumes game");
    require(state.events == std::vector<KeyEvent>{{0, 0, 0}}, "Finalization cannot send a gameplay key-down");
}

void footer_keeps_remaining_menu() {
    FakeUi state;
    const auto dispatch = actions(state);
    state.next = &state.parent;
    dispatch.run_script(nullptr, nullptr);
    require(state.active == &state.parent && state.events.empty() && state.paused && state.capture == 0x10,
            "Back to parent retains pause and input ownership");
    state.next = &state.dialog;
    dispatch.run_script(nullptr, nullptr);
    require(state.active == &state.dialog && state.events.empty(),
            "A dialog opened by a footer receives no synthetic key event");
    state.next = nullptr;
    dispatch.run_script(nullptr, nullptr);
    require(state.scripts == 3 && state.events.size() == 1 && !state.paused,
            "Only closing the final menu resumes the game");
}

void key_transition() {
    FakeUi state;
    const auto dispatch = actions(state);
    dispatch.press_key(155);
    require(state.events == std::vector<KeyEvent>{{0, 155, 1}, {0, 155, 0}},
            "Navigation in the same menu releases its key");
    state.events.clear();
    state.transition_on_key = true;
    state.next = &state.parent;
    dispatch.press_key(27);
    require(state.active == &state.parent && state.events == std::vector<KeyEvent>{{0, 27, 1}} &&
                state.paused,
            "A release cannot spill into another menu");
    state.events.clear();
    state.next = nullptr;
    dispatch.press_key(27);
    require(state.events == std::vector<KeyEvent>{{0, 27, 1}, {0, 27, 0}} && !state.active && !state.paused &&
                state.capture == 0,
            "Keyboard path also completes the last menu close");
}
} // namespace

int main() {
    try {
        footer_closes_last_menu();
        footer_keeps_remaining_menu();
        key_transition();
        std::cout << "Menu action tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
