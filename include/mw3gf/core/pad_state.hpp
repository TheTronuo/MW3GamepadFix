#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace mw3gf {
// These bits belong to our model. Neither backend's native enum escapes its adapter.
enum class Button : std::uint32_t {
    a = 1u << 0,
    b = 1u << 1,
    x = 1u << 2,
    y = 1u << 3,
    lb = 1u << 4,
    rb = 1u << 5,
    back = 1u << 6,
    start = 1u << 7,
    ls = 1u << 8,
    rs = 1u << 9,
    up = 1u << 10,
    down = 1u << 11,
    left = 1u << 12,
    right = 1u << 13
};
constexpr auto bit(Button button) noexcept {
    return static_cast<std::uint32_t>(button);
}
struct ButtonLabel {
    Button button;
    std::wstring_view label;
};
inline constexpr std::array button_labels{
    ButtonLabel{Button::a, L"A"},       ButtonLabel{Button::b, L"B"},
    ButtonLabel{Button::x, L"X"},       ButtonLabel{Button::y, L"Y"},
    ButtonLabel{Button::lb, L"LB"},     ButtonLabel{Button::rb, L"RB"},
    ButtonLabel{Button::back, L"BACK"}, ButtonLabel{Button::start, L"START"},
    ButtonLabel{Button::ls, L"LS"},     ButtonLabel{Button::rs, L"RS"},
    ButtonLabel{Button::up, L"UP"},     ButtonLabel{Button::down, L"DOWN"},
    ButtonLabel{Button::left, L"LEFT"}, ButtonLabel{Button::right, L"RIGHT"}};
struct Stick {
    float x{};
    float y{};
};
struct PadState {
    std::uint32_t buttons{};
    Stick left_stick{}, right_stick{};     // [-1, 1], positive Y points up.
    float left_trigger{}, right_trigger{}; // [0, 1]. Raw input: no gameplay deadzone yet.
    [[nodiscard]] bool held(Button button) const noexcept { return (buttons & bit(button)) != 0; }
};
struct InputSnapshot {
    bool connected{};
    std::wstring backend;
    std::wstring device_name;
    std::wstring device_id; // Used to reset edge state when the selected device changes.
    std::wstring diagnostic;
    PadState pad;
};
// Signed XInput ranges are asymmetric; preserve both endpoints exactly.
[[nodiscard]] constexpr float normalize_axis(std::int16_t value) noexcept {
    return value < 0 ? static_cast<float>(value) / 32768.0f : static_cast<float>(value) / 32767.0f;
}
[[nodiscard]] constexpr float normalize_trigger(std::uint8_t value) noexcept {
    return static_cast<float>(value) / 255.0f;
}
} // namespace mw3gf
