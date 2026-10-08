#include "mw3gf/input/backend.hpp"
#include <Windows.h>
#include <Xinput.h>
#include <array>
#include <optional>

namespace mw3gf {
namespace {
constexpr std::array mapping{std::pair{XINPUT_GAMEPAD_A, Button::a},
                             std::pair{XINPUT_GAMEPAD_B, Button::b},
                             std::pair{XINPUT_GAMEPAD_X, Button::x},
                             std::pair{XINPUT_GAMEPAD_Y, Button::y},
                             std::pair{XINPUT_GAMEPAD_LEFT_SHOULDER, Button::lb},
                             std::pair{XINPUT_GAMEPAD_RIGHT_SHOULDER, Button::rb},
                             std::pair{XINPUT_GAMEPAD_BACK, Button::back},
                             std::pair{XINPUT_GAMEPAD_START, Button::start},
                             std::pair{XINPUT_GAMEPAD_LEFT_THUMB, Button::ls},
                             std::pair{XINPUT_GAMEPAD_RIGHT_THUMB, Button::rs},
                             std::pair{XINPUT_GAMEPAD_DPAD_UP, Button::up},
                             std::pair{XINPUT_GAMEPAD_DPAD_DOWN, Button::down},
                             std::pair{XINPUT_GAMEPAD_DPAD_LEFT, Button::left},
                             std::pair{XINPUT_GAMEPAD_DPAD_RIGHT, Button::right}};
class XInputBackend final : public InputBackend {
  public:
    InputSnapshot poll() override {
        InputSnapshot result;
        result.backend = L"XInput 1.4";
        XINPUT_STATE state{};
        if (slot_ && XInputGetState(*slot_, &state) != ERROR_SUCCESS)
            slot_.reset();
        if (!slot_) {
            for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
                if (XInputGetState(i, &state) == ERROR_SUCCESS) {
                    slot_ = i;
                    break;
                }
            }
        }
        if (!slot_) {
            result.diagnostic = L"No XInput controller connected";
            return result;
        }
        result.connected = true;
        result.device_id = L"xinput:" + std::to_wstring(*slot_);
        result.device_name = L"Xbox-compatible controller / slot " + std::to_wstring(*slot_);
        const auto& native = state.Gamepad;
        for (const auto& [mask, button] : mapping)
            if (native.wButtons & mask)
                result.pad.buttons |= bit(button);
        result.pad.left_stick = {normalize_axis(native.sThumbLX), normalize_axis(native.sThumbLY)};
        result.pad.right_stick = {normalize_axis(native.sThumbRX), normalize_axis(native.sThumbRY)};
        result.pad.left_trigger = normalize_trigger(native.bLeftTrigger);
        result.pad.right_trigger = normalize_trigger(native.bRightTrigger);
        return result;
    }

  private:
    std::optional<DWORD> slot_;
};
} // namespace
std::unique_ptr<InputBackend> make_xinput_backend() {
    return std::make_unique<XInputBackend>();
}
} // namespace mw3gf
