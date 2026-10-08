#pragma once

#include <MinHook.h>
#include <string_view>
#include <type_traits>
#include <vector>

namespace mw3gf::game {

enum class HookId {
    menu_paint,
    item_paint,
    focusable,
    game_frame,
    create_cmd,
    binding_keys,
    localized_text,
    find_asset,
    text_command,
    lookup_glyph,
    mouse_move,
    remote_move,
    entity_use,
    handle_pic,
    render_text,
    renderer_release,
    register_material,
    text_width,
    decoded_text_width,
    text_count,
    count
};

[[nodiscard]] constexpr bool survives_stop(HookId id) noexcept {
    return id == HookId::game_frame || id == HookId::render_text || id == HookId::renderer_release ||
           id == HookId::register_material || id == HookId::text_count;
}

class HookSet {
  public:
    HookSet();
    ~HookSet();
    HookSet(const HookSet&) = delete;
    HookSet& operator=(const HookSet&) = delete;

    template <class Function> void add(HookId id, void* target, Function detour, Function& original) {
        static_assert(std::is_pointer_v<Function> && std::is_function_v<std::remove_pointer_t<Function>>);
        original = reinterpret_cast<Function>(create(id, target, reinterpret_cast<void*>(detour)));
    }

    void enable();
    void suspend_input();
    // Only a pinned, process-lifetime runtime may retain installed trampolines.
    void retain_for_process_exit() noexcept { retained_ = true; }

  private:
    struct Entry {
        HookId id;
        void* target;
    };
    void* create(HookId id, void* target, void* detour);
    std::vector<Entry> entries_;
    bool retained_{};
};

void check_hook(MH_STATUS status, std::string_view operation);

} // namespace mw3gf::game
