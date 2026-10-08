#pragma once
#include "mw3gf/core/button_icon.hpp"
#include "mw3gf/core/pad_state.hpp"
#include <optional>
namespace mw3gf {
struct GameplayBinding {
    std::uint32_t control;
    int down, up;
    unsigned glyph;
    std::string_view command;
};
inline constexpr std::uint32_t control_lt = 1u << 14, control_rt = 1u << 15;
// IDs verified in the supported PC binding table and Xbox buttons_default.cfg.
inline constexpr std::array gameplay_bindings{
    GameplayBinding{control_rt, 1, 2, icon_code(ButtonIcon::right_trigger), "+attack"},
    GameplayBinding{control_lt, 13, 14, icon_code(ButtonIcon::left_trigger), "+speed_throw"},
    GameplayBinding{bit(Button::rb), 5, 6, icon_code(ButtonIcon::right_bumper), "+frag"},
    GameplayBinding{bit(Button::lb), 7, 8, icon_code(ButtonIcon::left_bumper), "+smoke"},
    GameplayBinding{bit(Button::ls), 9, 10, icon_code(ButtonIcon::left_stick), "+breath_sprint"},
    GameplayBinding{bit(Button::x), 11, 12, icon_code(ButtonIcon::x), "+usereload"},
    GameplayBinding{bit(Button::b), 23, 24, icon_code(ButtonIcon::b), "+stance"},
    GameplayBinding{bit(Button::a), 25, 26, icon_code(ButtonIcon::a), "+gostand"},
    GameplayBinding{bit(Button::rs), 27, 28, icon_code(ButtonIcon::right_stick), "+melee_zoom"},
    GameplayBinding{bit(Button::up), 15, 16, icon_code(ButtonIcon::dpad_up), "+actionslot 1"},
    GameplayBinding{bit(Button::down), 17, 18, icon_code(ButtonIcon::dpad_down), "+actionslot 2"},
    GameplayBinding{bit(Button::left), 19, 20, icon_code(ButtonIcon::dpad_left), "+actionslot 3"},
    GameplayBinding{bit(Button::right), 21, 22, icon_code(ButtonIcon::dpad_right), "+actionslot 4"},
    GameplayBinding{bit(Button::y), 66, 0, icon_code(ButtonIcon::y), "weapnext"},
    GameplayBinding{bit(Button::start), 65, 0, icon_code(ButtonIcon::start), "togglemenu"},
};
struct GameplayEvent {
    std::size_t binding;
    bool down;
};
struct GameplayOutput {
    std::array<GameplayEvent, 32> events{};
    std::size_t count{};
    std::int8_t forward{}, right{};
    Stick look{};
    bool reset_camera{};
};
class GameplayAdapter {
  public:
    GameplayOutput update(const InputSnapshot& input, bool active);

  private:
    std::wstring device_, backend_;
    bool active_{}, left_blocked_{true}, right_blocked_{true};
    std::uint32_t held_{}, blocked_{};
};
// The same mappings drive input and action-based prompt substitution.
std::optional<unsigned> gameplay_glyph(std::string_view command) noexcept;
bool is_prompt_glyph(unsigned code) noexcept;
std::int8_t merge_movement(std::int8_t native, std::int8_t controller) noexcept;
} // namespace mw3gf
