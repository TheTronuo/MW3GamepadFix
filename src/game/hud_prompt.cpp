#include "mw3gf/game/hud_prompt.hpp"
namespace mw3gf::game {
const NativeLocalizeEntry* HudPromptCache::lookup(const NativeLocalizeEntry* original,
                                                 std::string_view key, std::string_view text,
                                                 std::string replacement) {
    if (!original || key.empty() || text.empty() || replacement.empty())
        return original;
    if (replacement == text)
        return original;
    std::scoped_lock lock(mutex_);
    auto cache_key = std::pair{std::string(key), replacement};
    const auto [found, inserted] = entries_.try_emplace(
        std::move(cache_key), Cached{std::move(replacement), {}});
    if (inserted) {
        found->second.entry.value = found->second.text.c_str();
        found->second.entry.name = found->first.first.c_str();
    }
    return &found->second.entry;
}
} // namespace mw3gf::game
