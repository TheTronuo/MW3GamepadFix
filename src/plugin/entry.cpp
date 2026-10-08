#include "mw3gf/game/runtime.hpp"
#include "mw3gf/game/settings.hpp"
#include "mw3gf/platform/windows.hpp"
#include <atomic>
#include <exception>
#include <fstream>
#include <stdexcept>

namespace {
HMODULE self{};
void record_error(const char* error) noexcept {
    try {
        std::ofstream(mw3gf::win::module_path(self).parent_path() / L"MW3GamepadFix.error.log", std::ios::app)
            << error << '\n';
    } catch (...) {
    }
}
} // namespace
extern "C" __declspec(dllexport) DWORD WINAPI MW3GamepadFix_Start(void* parameter) noexcept {
    try {
        const auto options = parameter ? *static_cast<const mw3gf::plugin::StartOptions*>(parameter)
                                       : mw3gf::plugin::StartOptions{};
        if (options.size != sizeof(options) || options.protocol != mw3gf::plugin::protocol_version ||
            static_cast<unsigned>(options.backend) >
                static_cast<unsigned>(mw3gf::plugin::BackendSelection::gameinput))
            throw std::runtime_error("Invalid loader options / protocol version");
        mw3gf::game::start(self, options);
        return ERROR_SUCCESS;
    } catch (const std::exception& error) {
        record_error(error.what());
        return ERROR_INVALID_DATA;
    } catch (...) {
        record_error("Unknown initialization error");
        return ERROR_UNHANDLED_EXCEPTION;
    }
}
extern "C" __declspec(dllexport) DWORD WINAPI MW3GamepadFix_Stop(void*) noexcept {
    try {
        mw3gf::game::stop();
        return ERROR_SUCCESS;
    } catch (const std::exception& error) {
        record_error(error.what());
        return ERROR_INVALID_FUNCTION;
    } catch (...) {
        record_error("Unknown shutdown error");
        return ERROR_UNHANDLED_EXCEPTION;
    }
}
#ifdef MW3GF_ASI
// Ultimate ASI Loader calls this after LoadLibrary returns, using delayed
// initialization by default. DllMain never installs hooks or opens input APIs.
extern "C" __declspec(dllexport) void InitializeASI() noexcept {
    static std::atomic_bool initialized{};
    if (initialized.exchange(true))
        return;
    try {
        // The proxy is also imported by the MP executable; leave it untouched.
        if (_wcsicmp(mw3gf::win::module_path().filename().c_str(), L"iw5sp.exe") != 0)
            return;
        const auto settings =
            mw3gf::game::read_settings(mw3gf::win::module_path(self).parent_path() / L"MW3GamepadFix.ini");
        if (!settings.enabled)
            return;
        mw3gf::plugin::StartOptions options;
        options.backend = settings.backend;
        (void)MW3GamepadFix_Start(&options);
    } catch (const std::exception& error) {
        record_error(error.what());
    } catch (...) {
        record_error("Unknown ASI bootstrap error");
    }
}
#endif
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        self = module;
    }
    if (reason == DLL_PROCESS_DETACH && reserved)
        mw3gf::game::abandon_for_process_exit();
    return TRUE; // Initialization is explicitly exported; no game/API work under the loader lock.
}
