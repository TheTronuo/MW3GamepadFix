#include "mw3gf/game/runtime_log.hpp"
#include <Windows.h>
#include <stdexcept>
namespace mw3gf::game {
RuntimeLog::RuntimeLog(const std::filesystem::path& path) : stream_(path, std::ios::app) {
    if (!stream_)
        throw std::runtime_error("Cannot open MW3GamepadFix.log beside the DLL");
}
void RuntimeLog::write(std::string_view text) noexcept {
    try {
        std::scoped_lock lock(mutex_);
        stream_ << GetTickCount64() << "  " << text << '\n';
        stream_.flush();
    } catch (...) { /* Logging must not affect game execution. */
    }
}
} // namespace mw3gf::game
