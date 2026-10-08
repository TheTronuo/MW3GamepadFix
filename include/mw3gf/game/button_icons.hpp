#pragma once
#include "mw3gf/core/prompt_style.hpp"
#include "mw3gf/game/native_memory.hpp"
#include "mw3gf/game/native_types.hpp"
#include <Windows.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <d3d9.h>
#include <mutex>
#include <string>
#include <vector>
#include <wrl/client.h>

namespace mw3gf::game {
// These resources stay alive until process exit, just like the pinned hook.
// No game-owned font, texture, or asset archive is modified.
class ButtonIcons {
  public:
    explicit ButtonIcons(HMODULE module, PromptStyle style = PromptStyle::x360);
    ~ButtonIcons();
    ButtonIcons(const ButtonIcons&) = delete;
    ButtonIcons& operator=(const ButtonIcons&) = delete;
    bool prepare(NativeFont* normal, void* ui_material = nullptr);
    Glyph* button_glyph(NativeFont* source, unsigned code) noexcept;
    std::string inline_text(std::string_view text) const;
    void* material(std::string_view name) noexcept;
    bool ready() const noexcept { return images_ready_.load(std::memory_order_acquire); }
    // Only after the native renderer has drained queued commands, outside DllMain.
    void release_gpu_resources() noexcept;
    [[nodiscard]] NativeFont* font() noexcept { return texture_ ? &font_ : nullptr; }

  private:
    struct MaterialCopy {
        alignas(8) std::array<std::byte, 0x88> material{};
        std::vector<std::array<std::byte, 16>> textures;
        std::vector<std::array<std::byte, 40>> images;
    };
    struct ButtonMaterial {
        MaterialCopy copy;
        Microsoft::WRL::ComPtr<IDirect3DTexture9> texture;
        std::string name;
    };
    void clone_material(void* source, MaterialCopy& target, IDirect3DTexture9* texture, unsigned width,
                        unsigned height);
    std::mutex gpu_mutex_;
    std::array<ButtonMaterial, 24> buttons_;
    std::array<std::array<Glyph, 24>, 129> metrics_{};
    std::atomic_bool images_ready_{};
    NativeFont font_{};
    std::vector<Glyph> glyphs_;
    const std::byte* pixels_{};
    unsigned width_{}, height_{};
    Microsoft::WRL::ComPtr<IDirect3DTexture9> texture_;
    MaterialCopy normal_, glow_;
};
} // namespace mw3gf::game
