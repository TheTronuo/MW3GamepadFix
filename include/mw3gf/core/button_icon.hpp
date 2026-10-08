#pragma once

namespace mw3gf {

// Values are the native Xbox glyph codes and the embedded icon resource IDs.
enum class ButtonIcon : unsigned {
    a = 1,
    b = 2,
    x = 3,
    y = 4,
    left_bumper = 5,
    right_bumper = 6,
    start = 14,
    back = 15,
    left_stick = 16,
    right_stick = 17,
    left_trigger = 18,
    right_trigger = 19,
    dpad_up = 20,
    dpad_down = 21,
    dpad_left = 22,
    dpad_right = 23
};

[[nodiscard]] constexpr unsigned icon_code(ButtonIcon icon) noexcept {
    return static_cast<unsigned>(icon);
}

[[nodiscard]] constexpr char icon_character(ButtonIcon icon) noexcept {
    return static_cast<char>(icon_code(icon));
}

} // namespace mw3gf
