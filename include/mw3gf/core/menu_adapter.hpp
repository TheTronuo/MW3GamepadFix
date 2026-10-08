#pragma once
#include "mw3gf/core/pad_state.hpp"
#include <array>
#include <cstddef>

namespace mw3gf {
enum class MenuAction {
    confirm,
    back,
    up,
    down,
    left,
    right,
    auxiliary_y,
    auxiliary_x,
    auxiliary_view,
    page_up,
    page_down,
    top,
    bottom
};
struct MenuContext {
    bool active{};
    std::uintptr_t identity{};
    std::uint32_t shortcut_buttons{};
};
struct MenuActions {
    std::array<MenuAction, 13> values{};
    std::size_t count{};
    void add(MenuAction action) noexcept { values[count++] = action; }
};
// A consumer of the shared PadState. Each action is a complete UI key pulse;
// it never leaves a keyboard key held and never sends gameplay commands.
class MenuAdapter {
  public:
    [[nodiscard]] MenuActions update(const InputSnapshot& input, MenuContext context, std::uint64_t now);

  private:
    [[nodiscard]] int direction(const PadState& pad) noexcept;
    std::wstring device_, backend_;
    std::uintptr_t menu_{};
    std::uint32_t previous_{}, blocked_{};
    int axis_x_{}, axis_y_{}, direction_{-1};
    std::uint64_t repeat_at_{};
    bool active_{}, block_direction_{};
};
} // namespace mw3gf
