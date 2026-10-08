#include "mw3gf/game/native_memory.hpp"
#include <Windows.h>
#include <cstdint>
namespace mw3gf::game {
bool readable(const void* address, std::size_t bytes) noexcept {
    auto position = reinterpret_cast<std::uintptr_t>(address);
    if (!position || !bytes || position + bytes < position)
        return false;
    const auto end = position + bytes;
    while (position < end) {
        MEMORY_BASIC_INFORMATION info{};
        if (!VirtualQuery(reinterpret_cast<void*>(position), &info, sizeof(info)) ||
            info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) ||
            !(info.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ |
                              PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
            return false;
        const auto next = reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize;
        if (next <= position)
            return false;
        position = next;
    }
    return true;
}
std::string safe_string(const char* text, std::size_t limit) {
    if (!text)
        return {};
    if (readable(text, limit)) {
        const auto* end = static_cast<const char*>(std::memchr(text, 0, limit));
        return end ? std::string(text, end) : std::string{};
    }
    std::string result;
    for (std::size_t i = 0; i < limit && readable(text + i, 1); ++i) {
        if (!text[i])
            return result;
        result += text[i];
    }
    return {}; // Reject unterminated/truncated strings.
}
} // namespace mw3gf::game
