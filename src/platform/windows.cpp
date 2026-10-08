#include "mw3gf/platform/windows.hpp"
#include <array>
#include <bcrypt.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace mw3gf::win {
[[noreturn]] void fail_last_error(std::string_view operation) {
    throw std::runtime_error(std::string(operation) + " failed; Win32=" + std::to_string(GetLastError()));
}
std::filesystem::path module_path(HMODULE module) {
    std::wstring buffer(32768, L'\0');
    const DWORD size = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!size || size >= buffer.size())
        fail_last_error("GetModuleFileNameW");
    buffer.resize(size);
    return buffer;
}
std::string encode(std::wstring_view value, unsigned code_page) {
    if (value.empty())
        return {};
    const int count = WideCharToMultiByte(code_page, 0, value.data(), static_cast<int>(value.size()), nullptr,
                                          0, nullptr, nullptr);
    if (count <= 0)
        fail_last_error("Text encode");
    std::string output(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(code_page, 0, value.data(), static_cast<int>(value.size()), output.data(), count,
                        nullptr, nullptr);
    return output;
}
std::string utf8(std::wstring_view value) {
    return encode(value, CP_UTF8);
}
std::string sha256(const std::filesystem::path& path) {
    struct Algorithm {
        BCRYPT_ALG_HANDLE value{};
        ~Algorithm() {
            if (value)
                BCryptCloseAlgorithmProvider(value, 0);
        }
    } algorithm;
    struct Hash {
        BCRYPT_HASH_HANDLE value{};
        ~Hash() {
            if (value)
                BCryptDestroyHash(value);
        }
    } hash;
    auto check = [](NTSTATUS status) {
        if (status < 0)
            throw std::runtime_error("BCrypt SHA256 failed");
    };
    check(BCryptOpenAlgorithmProvider(&algorithm.value, BCRYPT_SHA256_ALGORITHM, nullptr, 0));
    check(BCryptCreateHash(algorithm.value, &hash.value, nullptr, 0, nullptr, 0, 0));
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Cannot read executable for SHA256 validation");
    std::array<char, 65536> chunk{};
    while (file.read(chunk.data(), chunk.size()) || file.gcount() != 0) {
        check(BCryptHashData(hash.value, reinterpret_cast<PUCHAR>(chunk.data()),
                             static_cast<ULONG>(file.gcount()), 0));
    }
    if (file.bad())
        throw std::runtime_error("Executable read failed");
    std::array<UCHAR, 32> digest{};
    check(BCryptFinishHash(hash.value, digest.data(), static_cast<ULONG>(digest.size()), 0));
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (const auto byte : digest)
        output << std::setw(2) << static_cast<unsigned>(byte);
    return output.str();
}
} // namespace mw3gf::win
