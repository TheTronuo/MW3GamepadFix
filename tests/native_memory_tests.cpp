#include "mw3gf/game/native_memory.hpp"
#include <Windows.h>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
struct Pages {
    SYSTEM_INFO info{};
    char* data{};
    Pages() {
        GetSystemInfo(&info);
        data = static_cast<char*>(
            VirtualAlloc(nullptr, info.dwPageSize * 2ull, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        require(data != nullptr, "Cannot allocate memory test pages");
    }
    ~Pages() {
        if (data)
            VirtualFree(data, 0, MEM_RELEASE);
    }
};
} // namespace
int main() {
    using namespace mw3gf::game;
    try {
        Pages pages;
        const auto size = pages.info.dwPageSize;
        const int expected = 12345;
        std::memcpy(pages.data + 3, &expected, sizeof(expected));
        require(read_field<int>(pages.data, 3) == expected, "Unaligned field read failed");
        require(read_field<int>(nullptr, 0) == 0, "Null field read failed");
        require(!readable(nullptr, 1) && !readable(pages.data, 0), "Invalid range accepted");
        require(!readable(reinterpret_cast<void*>(std::numeric_limits<std::uintptr_t>::max() - 1), 8),
                "Overflowing range accepted");
        std::memcpy(pages.data + size - 2, "a\0", 2);
        DWORD previous{};
        require(VirtualProtect(pages.data + size, size, PAGE_NOACCESS, &previous) != 0,
                "Cannot protect page");
        require(safe_string(pages.data + size - 2, 64) == "a", "String near inaccessible page rejected");
        require(!readable(pages.data + size - 1, 2), "Range across inaccessible page accepted");
        require(read_field<int>(pages.data, size - 1) == 0, "Unreadable field did not fail safely");
        pages.data[size - 1] = 'b';
        require(safe_string(pages.data + size - 2, 64).empty(), "Truncated string accepted");
        require(safe_string(nullptr).empty(), "Null string accepted");
        std::cout << "Unaligned reads, page boundaries and truncated strings verified\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
