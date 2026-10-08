#include "mw3gf/game/hook_set.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace mw3gf::game {

void check_hook(MH_STATUS status, std::string_view operation) {
    if (status != MH_OK) {
        throw std::runtime_error(std::string(operation) + ": " + MH_StatusToString(status));
    }
}

HookSet::HookSet() {
    entries_.reserve(static_cast<std::size_t>(HookId::count));
    check_hook(MH_Initialize(), "MH_Initialize");
}

HookSet::~HookSet() {
    if (retained_)
        return;
    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
        MH_DisableHook(it->target);
        MH_RemoveHook(it->target);
    }
    MH_Uninitialize();
}

void* HookSet::create(HookId id, void* target, void* detour) {
    if (std::any_of(entries_.begin(), entries_.end(),
                    [=](const Entry& entry) { return entry.id == id || entry.target == target; }))
        throw std::logic_error("Duplicate hook identity or target");
    // Allocate the bookkeeping entry before acquiring the native hook.
    entries_.push_back({id, target});
    void* original{};
    const auto status = MH_CreateHook(target, detour, &original);
    if (status != MH_OK) {
        entries_.pop_back();
        check_hook(status, "MH_CreateHook");
    }
    return original;
}

void HookSet::enable() {
    for (const auto& entry : entries_)
        check_hook(MH_QueueEnableHook(entry.target), "MH_QueueEnableHook");
    check_hook(MH_ApplyQueued(), "MH_ApplyQueued");
}

void HookSet::suspend_input() {
    for (const auto& entry : entries_) {
        if (!survives_stop(entry.id))
            check_hook(MH_QueueDisableHook(entry.target), "MH_QueueDisableHook");
    }
    check_hook(MH_ApplyQueued(), "MH_ApplyQueued");
}

} // namespace mw3gf::game
