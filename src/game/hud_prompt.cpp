#include "mw3gf/game/hud_prompt.hpp"
#include "mw3gf/core/gameplay_prompts.hpp"
namespace mw3gf::game {
const NativeLocalizeEntry* HudPromptCache::lookup(const NativeLocalizeEntry* original,
                                                 std::string_view key, std::string_view text,
                                                 bool controller_mode) {
    if (!controller_mode || !original || key != sdv_prompt_key || text.empty())
        return original;
    auto replacement = controller_instruction(key, text);
    if (replacement == text)
        return original;
    std::scoped_lock lock(mutex_);
    const auto [found, inserted] = entries_.try_emplace(
        std::pair{std::string(key), std::string(text)}, Cached{std::move(replacement), {}});
    if (inserted) {
        found->second.entry.value = found->second.text.c_str();
        found->second.entry.name = found->first.first.c_str();
    }
    return &found->second.entry;
}
} // namespace mw3gf::game
