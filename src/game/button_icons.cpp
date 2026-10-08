#include "mw3gf/game/button_icons.hpp"
#include "mw3gf/core/gameplay_adapter.hpp"
#include "mw3gf/core/gameplay_prompts.hpp"
#include <algorithm>
#include <cstring>
#include <span>
#include <stdexcept>
#include <wrl/client.h>

namespace mw3gf::game {
namespace {
template <class T> T read(const void* data, std::size_t offset) {
    T value{};
    std::memcpy(&value, static_cast<const std::byte*>(data) + offset, sizeof(T));
    return value;
}
template <class T, std::size_t N> void write(std::array<std::byte, N>& data, std::size_t offset, T value) {
    std::memcpy(data.data() + offset, &value, sizeof(T));
}
void require(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
unsigned reference_width(const Glyph& glyph) {
    return std::max<unsigned>(glyph.advance, glyph.width);
}
} // namespace

ButtonIcons::ButtonIcons(HMODULE module, PromptStyle style) {
    const int resource_id = style == PromptStyle::ps3 ? 102 : 101;
    const auto resource = FindResourceW(module, MAKEINTRESOURCEW(resource_id), RT_RCDATA);
    require(resource != nullptr, "Button icon resource missing");
    const auto size = SizeofResource(module, resource);
    const auto* data = static_cast<const std::byte*>(LockResource(LoadResource(module, resource)));
    require(data && size >= 24 && std::memcmp(data, "BGP1", 4) == 0, "Invalid button icon resource");
    width_ = read<unsigned>(data, 4);
    height_ = read<unsigned>(data, 8);
    const auto count = read<unsigned>(data, 16), pixels = read<unsigned>(data, 20);
    require(width_ == 256 && height_ == 256 && count == 16 && pixels == width_ * height_ * 4 &&
                size == 24 + count * sizeof(Glyph) + pixels && read<unsigned>(data, 12) == 33,
            "Button icon resource size mismatch");
    // Native lookup requires 96 ASCII slots. They contain no letter pixels.
    glyphs_.resize(96 + count);
    for (unsigned i = 0; i < 96; ++i)
        glyphs_[i].code = static_cast<std::uint16_t>(32 + i);
    std::memcpy(glyphs_.data() + 96, data + 24, count * sizeof(Glyph));
    for (unsigned i = 96; i < glyphs_.size(); ++i) {
        const auto& glyph = glyphs_[i];
        require(is_prompt_glyph(glyph.code) && glyph.width && glyph.height && glyph.width <= 60 &&
                    glyph.height <= 60 && glyph.s0 >= 0 && glyph.t0 >= 0 && glyph.s1 <= 1 && glyph.t1 <= 1 &&
                    (i == 96 || glyph.code > glyphs_[i - 1].code),
                "Invalid button glyph");
        auto& button = buttons_[glyph.code];
        button.name = "mw3gf/p" + std::string(1, static_cast<char>('0' + glyph.code / 10)) +
                      static_cast<char>('0' + glyph.code % 10);
        for (int height = 1; height <= 128; ++height) {
            auto& metric = metrics_[height][glyph.code];
            metric = glyph;
            metric.advance = static_cast<std::uint8_t>(
                std::clamp(image_dimension(image_dimension_byte(reference_width(glyph)), height), 1, 255));
        }
    }
    pixels_ = data + 24 + count * sizeof(Glyph);
    font_ = {"mw3gf/buttons", 33, static_cast<int>(glyphs_.size()), nullptr, nullptr, glyphs_.data()};
}
ButtonIcons::~ButtonIcons() {
    release_gpu_resources();
}
void ButtonIcons::release_gpu_resources() noexcept {
    std::scoped_lock lock(gpu_mutex_);
    images_ready_.store(false, std::memory_order_release);
    font_.material = nullptr;
    font_.glow_material = nullptr;
    for (auto& button : buttons_)
        if (button.texture) {
            button.texture.Reset();
        }
    if (texture_) {
        texture_.Reset();
    }
}
void ButtonIcons::clone_material(void* source, MaterialCopy& target, IDirect3DTexture9* texture,
                                 unsigned width, unsigned height) {
    require(readable(source, target.material.size()), "Native material unreadable");
    const auto count = read<std::uint8_t>(source, 0x56);
    const auto* table = read<const std::byte*>(source, 0x68);
    require(count > 0 && count <= 8 && readable(table, count * 16u),
            "Native material texture layout mismatch");
    std::memcpy(target.material.data(), source, target.material.size());
    target.textures.resize(count);
    target.images.resize(count);
    for (unsigned i = 0; i < count; ++i) {
        std::memcpy(target.textures[i].data(), table + i * 16, 16);
        const auto* image = read<const std::byte*>(table + i * 16, 8);
        require(readable(image, 40) && read<std::uint8_t>(image, 8) == 3,
                "Native material image layout mismatch");
        std::memcpy(target.images[i].data(), image, 40);
        write(target.images[i], 0, texture);
        write(target.images[i], 0x18, static_cast<std::uint16_t>(width));
        write(target.images[i], 0x1A, static_cast<std::uint16_t>(height));
        write(target.images[i], 0x1E, static_cast<std::uint8_t>(1));
        write(target.images[i], 0x20, "mw3gf/buttons");
        write(target.textures[i], 8, target.images[i].data());
        write(target.textures[i], 6, static_cast<std::uint8_t>(0x62));
    }
    write(target.material, 0, "mw3gf/buttons");
    write(target.material, 0x68, target.textures.data());
}
bool ButtonIcons::prepare(NativeFont* normal, void* ui_material) {
    std::scoped_lock lock(gpu_mutex_);
    if (ready())
        return true;
    if (!readable(normal, sizeof(NativeFont)) || !readable(normal->material, 0x88))
        return false;
    const auto* table = read<const std::byte*>(normal->material, 0x68);
    if (!readable(table, 16))
        return false;
    const auto* image = read<const std::byte*>(table, 8);
    if (!readable(image, 40) || read<std::uint8_t>(image, 8) != 3)
        return false;
    auto* original = read<IDirect3DTexture9*>(image, 0);
    if (!original)
        return false;
    Microsoft::WRL::ComPtr<IDirect3DDevice9> device;
    require(SUCCEEDED(original->GetDevice(&device)), "Cannot obtain native D3D9 device");
    if (!texture_) {
        Microsoft::WRL::ComPtr<IDirect3DTexture9> texture;
        require(SUCCEEDED(device->CreateTexture(width_, height_, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                                &texture, nullptr)),
                "Cannot create button atlas");
        D3DLOCKED_RECT locked{};
        require(SUCCEEDED(texture->LockRect(0, &locked, nullptr, 0)), "Cannot lock button atlas");
        for (unsigned y = 0; y < height_; ++y)
            std::memcpy(static_cast<std::byte*>(locked.pBits) + y * static_cast<std::size_t>(locked.Pitch),
                        pixels_ + y * width_ * 4, width_ * 4u);
        require(SUCCEEDED(texture->UnlockRect(0)), "Cannot upload button atlas");
        clone_material(normal->material, normal_, texture.Get(), width_, height_);
        clone_material(normal->glow_material ? normal->glow_material : normal->material, glow_, texture.Get(),
                       width_, height_);
        font_.material = normal_.material.data();
        font_.glow_material = glow_.material.data();
        texture_ = std::move(texture);
    }
    // A stock alpha-blended UI material, independent of localized font layout.
    if (!readable(ui_material, 0x88))
        return false;
    for (const auto& glyph : std::span(glyphs_).subspan(96)) {
        auto& button = buttons_[glyph.code];
        if (button.texture)
            continue;
        const unsigned width = reference_width(glyph), height = glyph.height;
        Microsoft::WRL::ComPtr<IDirect3DTexture9> texture;
        require(SUCCEEDED(device->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                                &texture, nullptr)),
                "Cannot create button image");
        D3DLOCKED_RECT locked{};
        require(SUCCEEDED(texture->LockRect(0, &locked, nullptr, 0)), "Cannot lock button image");
        const unsigned sx = static_cast<unsigned>(glyph.s0 * width_),
                       sy = static_cast<unsigned>(glyph.t0 * height_);
        const unsigned inset = (width - glyph.width) / 2;
        for (unsigned y = 0; y < height; ++y) {
            auto* row = static_cast<std::byte*>(locked.pBits) + y * static_cast<std::size_t>(locked.Pitch);
            std::memset(row, 0, width * 4u);
            std::memcpy(row + inset * 4, pixels_ + ((sy + y) * width_ + sx) * 4, glyph.width * 4u);
        }
        require(SUCCEEDED(texture->UnlockRect(0)), "Cannot upload button image");
        clone_material(ui_material, button.copy, texture.Get(), width, height);
        write(button.copy.material, 0, button.name.c_str());
        button.texture = std::move(texture);
    }
    images_ready_.store(true, std::memory_order_release);
    return true;
}
Glyph* ButtonIcons::button_glyph(NativeFont* source, unsigned code) noexcept {
    if (!is_prompt_glyph(code) || !readable(source, sizeof(NativeFont)) || source == &font_ ||
        source->pixel_height < 1 || source->pixel_height > 128)
        return nullptr;
    return &metrics_[source->pixel_height][code];
}
std::string ButtonIcons::inline_text(std::string_view text) const {
    std::string result;
    char color = '7';
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (const auto bytes = inline_material_bytes(text, i)) {
            result.append(text.substr(i, bytes));
            i += bytes - 1;
            continue;
        }
        if (text[i] == '^' && i + 1 < text.size() && text[i + 1] >= '0' && text[i + 1] <= ';') {
            color = text[i + 1];
            result.append(text.substr(i, 2));
            ++i;
            continue;
        }
        const auto code = static_cast<unsigned char>(text[i]);
        if (!is_prompt_glyph(code)) {
            result += text[i];
            continue;
        }
        const auto found = std::find_if(glyphs_.begin() + 96, glyphs_.end(),
                                        [code](const Glyph& glyph) { return glyph.code == code; });
        if (found == glyphs_.end()) {
            result += text[i];
            continue;
        }
        if (color != '7')
            result += "^7";
        result += inline_button_record(code, reference_width(*found), found->height);
        if (color != '7') {
            result += '^';
            result += color;
        }
    }
    return result;
}
void* ButtonIcons::material(std::string_view name) noexcept {
    if (!ready())
        return nullptr;
    for (auto& button : buttons_)
        if (button.texture && name == button.name)
            return button.copy.material.data();
    return nullptr;
}
} // namespace mw3gf::game
