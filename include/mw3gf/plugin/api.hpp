#pragma once
#include <cstdint>
namespace mw3gf::plugin {
inline constexpr std::uint32_t protocol_version = 1;
enum class BackendSelection : std::uint32_t { automatic, xinput, gameinput };
struct StartOptions {
    std::uint32_t size{sizeof(StartOptions)};
    std::uint32_t protocol{protocol_version};
    BackendSelection backend{BackendSelection::automatic};
    std::uint32_t demo{};
};
static_assert(sizeof(StartOptions) == 16);
} // namespace mw3gf::plugin
