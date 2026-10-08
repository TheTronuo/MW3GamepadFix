#include "mw3gf/game/use_hold.hpp"
#include "mw3gf/game/native_memory.hpp"
#include <algorithm>
#include <limits>
namespace mw3gf::game {
const UseHoldRule* native_use_hold_rule(const void* target, WeaponClass weapon_class,
                                       ServerConfigString config_string) {
    using namespace use_layout;
    if (!readable(target, entity_number + 2))
        return nullptr;
    UseTarget description{read_field<std::uint8_t>(target, entity_type),
                          read_field<std::uint32_t>(target, weapon)};
    if (description.entity_type == 3 && description.weapon && weapon_class)
        description.weapon_class = weapon_class(description.weapon, false);
    std::array<char, 1024> hint{};
    if ((description.entity_type == 0 || description.entity_type == 5) && config_string &&
        read_field<int>(target, cursor_hint) != 0) {
        const auto index = read_field<std::uint8_t>(target, hint_index);
        if (index < hint_config_count) {
            config_string(hint_config_base + index, hint.data(), static_cast<int>(hint.size()));
            // Reject an unterminated configstring rather than examining its tail.
            if (const auto end = std::find(hint.begin(), hint.end(), '\0'); end != hint.end())
                description.hint_key = std::string_view(hint.data(), static_cast<std::size_t>(end - hint.begin()));
        }
    }
    return find_use_hold_rule(description);
}
void UseHold::record_command(std::uint32_t time, bool controller_use) {
    std::scoped_lock lock(mutex_);
    commands_[next_] = {time, controller_use, true};
    next_ = (next_ + 1) % commands_.size();
}
bool UseHold::defer(const void* player, const void* target, std::uintptr_t caller_rva,
                     std::uint32_t now, bool gameplay_active, const UseHoldRule* rule) {
    using namespace use_layout;
    if (!rule || caller_rva != timed_entity_use_return_rva || !readable(target, entity_number + 2))
        return false;
    const auto* state = read_field<void*>(player, client);
    if (!readable(state, use_start + 4) || !(read_field<std::uint32_t>(state, buttons) & usereload))
        return false;
    const auto handle = read_field<std::uint16_t>(state, use_handle);
    const auto number = read_field<std::uint16_t>(target, entity_number);
    if (!handle || number >= 0x7FF || handle != number + 1)
        return false;
    const auto command_time = read_field<std::uint32_t>(state, command);
    bool controller_use{};
    {
        std::scoped_lock lock(mutex_);
        for (std::size_t i = 0; i < commands_.size(); ++i) {
            const auto& source = commands_[(next_ + commands_.size() - 1 - i) % commands_.size()];
            if (source.valid && source.time == command_time) {
                controller_use = source.controller_use;
                break;
            }
        }
    }
    if (!controller_use)
        return false;
    if (!gameplay_active)
        return true;
    const auto elapsed = now - read_field<std::uint32_t>(state, use_start);
    return elapsed < rule->hold_ms ||
           elapsed > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
}
} // namespace mw3gf::game
