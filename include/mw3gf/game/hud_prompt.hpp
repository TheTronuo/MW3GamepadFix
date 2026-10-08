#pragma once
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
namespace mw3gf::game {
// Serialized LocalizeEntry on the supported x64 executable.
struct NativeLocalizeEntry {
    const char* value;
    const char* name;
};
inline constexpr int localize_asset_type = 27;
// Return address of the formatter's direct DB_FindXAssetHeader call.
inline constexpr std::uintptr_t hud_localize_return_rva = 0x289B7F;
inline constexpr std::string_view sdv_prompt_key = "NY_HARBOR_PLATFORM_HINT_DRIVE_SDV_3";
[[nodiscard]] constexpr bool is_sdv_hud_lookup(int type, std::string_view key,
                                               std::uintptr_t caller_rva) noexcept {
    return type == localize_asset_type && caller_rva == hud_localize_return_rva && key == sdv_prompt_key;
}
// Private, stable LocalizeEntry views; the database-owned asset is never changed.
class HudPromptCache {
  public:
    const NativeLocalizeEntry* lookup(const NativeLocalizeEntry* original, std::string_view key,
                                      std::string_view text, bool controller_mode);
  private:
    struct Cached {
        std::string text;
        NativeLocalizeEntry entry{};
    };
    std::mutex mutex_;
    std::map<std::pair<std::string, std::string>, Cached> entries_;
};
} // namespace mw3gf::game
