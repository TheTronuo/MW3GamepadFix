#pragma once
#include <cstddef>
#include <cstring>
#include <string>
#include <type_traits>
namespace mw3gf::game {
[[nodiscard]] bool readable(const void* address, std::size_t bytes) noexcept;
[[nodiscard]] std::string safe_string(const char* text, std::size_t limit = 1024);
template <class T> [[nodiscard]] T read_field(const void* object, std::size_t offset) noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    std::remove_cv_t<T> value{};
    if (!object)
        return value;
    const auto* bytes = static_cast<const std::byte*>(object);
    if (readable(bytes + offset, sizeof(T)))
        std::memcpy(&value, bytes + offset, sizeof(T));
    return value;
}
} // namespace mw3gf::game
