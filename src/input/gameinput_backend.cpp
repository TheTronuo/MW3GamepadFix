#include "mw3gf/input/backend.hpp"
#include <GameInput.h>
#include <Windows.h>
#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <wrl/client.h>

namespace mw3gf {
namespace {
namespace gi = GameInput::v3;
using Microsoft::WRL::ComPtr;
constexpr std::array mapping{std::pair{gi::GameInputGamepadA, Button::a},
                             std::pair{gi::GameInputGamepadB, Button::b},
                             std::pair{gi::GameInputGamepadX, Button::x},
                             std::pair{gi::GameInputGamepadY, Button::y},
                             std::pair{gi::GameInputGamepadLeftShoulder, Button::lb},
                             std::pair{gi::GameInputGamepadRightShoulder, Button::rb},
                             std::pair{gi::GameInputGamepadView, Button::back},
                             std::pair{gi::GameInputGamepadMenu, Button::start},
                             std::pair{gi::GameInputGamepadLeftThumbstick, Button::ls},
                             std::pair{gi::GameInputGamepadRightThumbstick, Button::rs},
                             std::pair{gi::GameInputGamepadDPadUp, Button::up},
                             std::pair{gi::GameInputGamepadDPadDown, Button::down},
                             std::pair{gi::GameInputGamepadDPadLeft, Button::left},
                             std::pair{gi::GameInputGamepadDPadRight, Button::right}};
std::wstring utf8(const char* text) {
    if (!text || !*text)
        return L"GameInput controller";
    const int size = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (size <= 1)
        return L"GameInput controller";
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text, -1, result.data(), size);
    result.pop_back();
    return result;
}
class GameInputBackend final : public InputBackend {
  public:
    GameInputBackend() {
        const HRESULT hr = gi::GameInputCreate(input_.GetAddressOf());
        if (FAILED(hr)) {
            std::ostringstream error;
            error << "GameInput v3 unavailable (0x" << std::hex << static_cast<unsigned long>(hr)
                  << "). SDK restored, but a compatible GameInput runtime is required.";
            throw std::runtime_error(error.str());
        }
        // The standalone probe also reads input without owning the foreground window.
        input_->SetFocusPolicy(gi::GameInputEnableBackgroundInput);
    }
    InputSnapshot poll() override {
        InputSnapshot result;
        result.backend = L"GameInput / API v3 / SDK 3.5.283";
        if (device_ && !(device_->GetDeviceStatus() & gi::GameInputDeviceConnected))
            device_.Reset();
        ComPtr<gi::IGameInputReading> reading;
        const HRESULT hr =
            input_->GetCurrentReading(gi::GameInputKindGamepad, device_.Get(), reading.GetAddressOf());
        if (FAILED(hr)) {
            result.diagnostic = L"Waiting for a GameInput gamepad reading";
            return result;
        }
        ComPtr<gi::IGameInputDevice> candidate;
        reading->GetDevice(candidate.GetAddressOf());
        if (!(candidate->GetDeviceStatus() & gi::GameInputDeviceConnected)) {
            device_.Reset();
            result.diagnostic = L"GameInput controller disconnected";
            return result;
        }
        if (candidate.Get() != device_.Get()) {
            device_ = candidate;
            ++selection_id_;
        }
        gi::GameInputGamepadState state{};
        if (!reading->GetGamepadState(&state)) {
            result.diagnostic = L"Reading has no gamepad state";
            return result;
        }
        result.connected = true;
        result.device_id = L"gameinput:" + std::to_wstring(selection_id_);
        const gi::GameInputDeviceInfo* info = nullptr;
        result.device_name = SUCCEEDED(device_->GetDeviceInfo(&info)) && info ? utf8(info->displayName)
                                                                              : L"GameInput controller";
        for (const auto& [mask, button] : mapping)
            if (state.buttons & mask)
                result.pad.buttons |= bit(button);
        result.pad.left_stick = {std::clamp(state.leftThumbstickX, -1.0f, 1.0f),
                                 std::clamp(state.leftThumbstickY, -1.0f, 1.0f)};
        result.pad.right_stick = {std::clamp(state.rightThumbstickX, -1.0f, 1.0f),
                                  std::clamp(state.rightThumbstickY, -1.0f, 1.0f)};
        result.pad.left_trigger = std::clamp(state.leftTrigger, 0.0f, 1.0f);
        result.pad.right_trigger = std::clamp(state.rightTrigger, 0.0f, 1.0f);
        return result;
    }

  private:
    ComPtr<gi::IGameInput> input_;
    ComPtr<gi::IGameInputDevice> device_;
    unsigned selection_id_{};
};
} // namespace
std::unique_ptr<InputBackend> make_gameinput_backend() {
    return std::make_unique<GameInputBackend>();
}
} // namespace mw3gf
