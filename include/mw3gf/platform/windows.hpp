#pragma once
#include <Windows.h>
#include <filesystem>
#include <string>
#include <string_view>

namespace mw3gf::win {
[[nodiscard]] std::filesystem::path module_path(HMODULE module = nullptr);
[[nodiscard]] std::string sha256(const std::filesystem::path& file);
[[nodiscard]] std::string utf8(std::wstring_view value);
[[nodiscard]] std::string encode(std::wstring_view value, unsigned code_page);
[[noreturn]] void fail_last_error(std::string_view operation);
} // namespace mw3gf::win
