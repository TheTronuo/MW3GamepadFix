#pragma once
#include "mw3gf/core/use_policy.hpp"
#include "mw3gf/game/use_profile.hpp"
#include <mutex>
namespace mw3gf::game {
using WeaponClass = int (*)(std::uint32_t, bool);
using ServerConfigString = void (*)(int, char*, int);
[[nodiscard]] const UseHoldRule* native_use_hold_rule(const void* target, WeaponClass weapon_class,
                                                      ServerConfigString config_string);
// Associate controller input with the queued native command, not the current
// physical X state. Keep the native selected handle and start time intact.
class UseHold {
  public:
    void record_command(std::uint32_t time, bool controller_use);
    [[nodiscard]] bool defer(const void* player, const void* target, std::uintptr_t caller_rva,
                             std::uint32_t now, bool gameplay_active, const UseHoldRule* rule);
  private:
    struct CommandSource { std::uint32_t time{}; bool controller_use{}, valid{}; };
    std::array<CommandSource, 128> commands_{};
    std::size_t next_{};
    std::mutex mutex_;
};
} // namespace mw3gf::game
