#pragma once
#include "mw3gf/game/build_profile.hpp"
#include "mw3gf/game/gameplay_profile.hpp"
#include "mw3gf/game/native_memory.hpp"
#include "mw3gf/game/native_types.hpp"
#include <span>
namespace mw3gf::game {
using ReadCharacter = unsigned (*)(const char**, int, int);
using GameFrame = void (*)(int, float);
using CreateCmd = void* (*)(void*, int);
using MouseMove = void (*)(void*, float);
using RemoteMove = void (*)(int, void*);
using BindingKeys = int (*)(int, const char*, char*);
using LocalizedText = const char* (*)(const char*);
using FindAsset = void* (*)(int, const char*, int);
using LookupGlyph = Glyph* (*)(NativeFont*, unsigned);
using TextCommand = void* (*)(const char*, int, NativeFont*, float, float, float, float, float, const float*,
                              int, int, bool);
using HandlePic = void (*)(void*, float, float, float, float, int, int, const float*, void*);
using RenderText = void (*)(void*);
using RendererRelease = void (*)(int);
using RegisterMaterial = void* (*)(const char*, int);
using TextWidth = int (*)(const char*, int, NativeFont*);
using DecodedTextWidth = int (*)(const char*, int, NativeFont*, int);
using TextCount = int (*)(const char*);
struct OriginalFunctions {
    MenuPaint menu_paint{};
    ItemPaint item_paint{};
    FocusableItem focusable{};
    GameFrame game_frame{};
    CreateCmd create_cmd{};
    BindingKeys binding_keys{};
    LocalizedText localized_text{};
    FindAsset find_asset{};
    TextCommand text_command{};
    LookupGlyph lookup_glyph{};
    MouseMove mouse_move{};
    RemoteMove remote_move{};
    HandlePic handle_pic{};
    RenderText render_text{};
    RendererRelease renderer_release{};
    RegisterMaterial register_material{};
    TextWidth text_width{};
    DecodedTextWidth decoded_text_width{};
    TextCount text_count{};
};
class EngineApi {
  public:
    explicit EngineApi(OriginalFunctions& originals);
    [[nodiscard]] std::uintptr_t base() const noexcept { return base_; }
    [[nodiscard]] OriginalFunctions& originals() const noexcept { return originals_; }
    template <class T> [[nodiscard]] T function(std::uintptr_t rva) const noexcept {
        return reinterpret_cast<T>(base_ + rva);
    }
    template <class T> [[nodiscard]] T read(std::uintptr_t rva) const noexcept {
        static_assert(std::is_trivially_copyable_v<T>);
        std::remove_cv_t<T> value{};
        std::memcpy(&value, address(rva), sizeof(T));
        return value;
    }
    [[nodiscard]] void* address(std::uintptr_t rva) const noexcept {
        return reinterpret_cast<void*>(base_ + rva);
    }

  private:
    void verify(std::uintptr_t rva, std::span<const std::uint8_t> signature, std::string_view name) const;
    std::uintptr_t base_;
    OriginalFunctions& originals_;
};
} // namespace mw3gf::game
