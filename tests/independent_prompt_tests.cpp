#include "mw3gf/core/gameplay_prompts.hpp"
#include "mw3gf/game/button_icons.hpp"
#include <Windows.h>
#include <array>
#include <cstring>
#include <cwchar>
#include <d3d9.h>
#include <iostream>
#include <stdexcept>
#include <wrl/client.h>

namespace {
void require(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
template <class T, std::size_t N> void put(std::array<std::byte, N>& data, std::size_t offset, T value) {
    std::memcpy(data.data() + offset, &value, sizeof(T));
}
struct Surface {
    std::array<std::byte, 0x88> material{};
    std::array<std::byte, 16> table{};
    std::array<std::byte, 40> image{};
    explicit Surface(IDirect3DTexture9* texture) {
        put(material, 0, "unrelated/localized/material");
        put(material, 0x56, std::uint8_t{1});
        put(material, 0x68, table.data());
        put(table, 8, image.data());
        put(image, 0, texture);
        put(image, 8, std::uint8_t{3});
        put(image, 0x18, std::uint16_t{17});
        put(image, 0x1A, std::uint16_t{19});
        put(image, 0x20, "arbitrary/font/layout");
    }
};
unsigned read_utf8(const char** position, void*) {
    const auto byte = static_cast<unsigned char>(*(*position)++);
    if (byte < 0x80)
        return byte;
    const auto second = static_cast<unsigned char>(*(*position)++);
    return ((byte & 31u) << 6) | (second & 63u);
}
int advance(unsigned code, void*) {
    return code == 0x0416 ? 11 : 7;
}
} // namespace
int wmain(int argc, wchar_t** argv) {
    HWND window{};
    HMODULE module{};
    try {
        require(argc == 3, "ASI resource path and prompt style required");
        require(std::wcscmp(argv[2], L"x360") == 0 || std::wcscmp(argv[2], L"ps3") == 0,
                "Unknown prompt style");
        module = LoadLibraryExW(argv[1], nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
        require(module != nullptr, "Cannot open ASI resources without running plugin");
        using namespace mw3gf;
        using namespace mw3gf::game;
        const auto style = std::wcscmp(argv[2], L"ps3") == 0 ? PromptStyle::ps3 : PromptStyle::x360;
        const bool ps3 = style == PromptStyle::ps3;
        ButtonIcons icons(module, style);
        window = CreateWindowExW(0, L"STATIC", L"MW3 independent prompt resource test", WS_OVERLAPPED, 0, 0,
                                 32, 32, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        require(window != nullptr, "Cannot create hidden D3D test window");
        Microsoft::WRL::ComPtr<IDirect3D9> d3d;
        d3d.Attach(Direct3DCreate9(D3D_SDK_VERSION));
        require(d3d != nullptr, "D3D9 unavailable");
        D3DPRESENT_PARAMETERS present{};
        present.Windowed = TRUE;
        present.SwapEffect = D3DSWAPEFFECT_DISCARD;
        present.BackBufferWidth = 32;
        present.BackBufferHeight = 32;
        present.BackBufferFormat = D3DFMT_UNKNOWN;
        present.hDeviceWindow = window;
        Microsoft::WRL::ComPtr<IDirect3DDevice9> device;
        require(SUCCEEDED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
                                            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present, &device)),
                "Cannot create D3D9 test device");
        Microsoft::WRL::ComPtr<IDirect3DTexture9> source;
        require(SUCCEEDED(
                    device->CreateTexture(17, 19, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &source, nullptr)),
                "Cannot create unrelated font texture");
        Surface font_surface(source.Get()), ui_surface(source.Get());
        const auto before = font_surface.material;
        NativeFont font{"other-language-font", 47, 3, font_surface.material.data(), nullptr, nullptr};
        const auto original = font;
        require(!icons.prepare(&font), "Wait for a UI material before publishing image readiness");
        require(!icons.ready(), "No partially prepared image set exposed");
        require(icons.prepare(&font, ui_surface.material.data()) && icons.ready(),
                "Independent images uploaded on real D3D9 device");
        require(font_surface.material == before && std::memcmp(&font, &original, sizeof(font)) == 0,
                "Original font/material are never modified");
        require(!icons.material("ui_cursor") && !icons.material("mw3gf/p99"),
                "Foreign and invalid material names fall through");
        const std::string text = "\xD0\x96 Press ^3\x03^7 P90";
        const auto inline_text = icons.inline_text(text);
        require(inline_text.starts_with("\xD0\x96 Press ^3^7^") && inline_text.ends_with("^3^7 P90"),
                "Text bytes, name and surrounding colors preserved");
        require(!contains_prompt_glyph(inline_text) && icons.inline_text(inline_text) == inline_text,
                "Image transformation is idempotent and skips binary fields");
        require(prompt_visible_characters(inline_text, read_utf8, nullptr) ==
                    prompt_visible_characters(text, read_utf8, nullptr),
                "FX reveal duration unchanged after encoding images");
        const std::string weapon = std::string("^") + char(1) + char(48) + char(48) + char(3) + "gun";
        require(icons.inline_text(weapon) == weapon && !contains_prompt_glyph(weapon),
                "Weapon image never becomes a gamepad glyph");
        struct WidthContext {
            ButtonIcons* icons;
            NativeFont* font;
        } width_context{&icons, &font};
        const auto prompt_advance = [](unsigned code, void* opaque) -> int {
            const auto& state = *static_cast<WidthContext*>(opaque);
            if (const auto metric = state.icons->button_glyph(state.font, code))
                return metric->advance;
            return advance(code, nullptr);
        };
        for (const int height : {12, 33, 47, 96}) {
            font.pixel_height = height;
            for (const std::string prefix :
                 {"Press ^3\x03 ^7to swap for", "Нажмите ^3\x03 ^7для замены", "اضغط ^3\x03 ^7لاستبدال"}) {
                const auto measured =
                    prompt_text_width(prefix, 0, height, read_utf8, prompt_advance, &width_context);
                const auto rendered = prompt_text_width(icons.inline_text(prefix), 0, height, read_utf8,
                                                        prompt_advance, &width_context);
                require(measured && measured == rendered,
                        "Pickup name offset includes the complete rendered button width in EN/RU/AR");
                for (const std::string name : {" AK-47", " AK-47 ACOG"}) {
                    require(prompt_text_width(prefix + name, 0, height, read_utf8, prompt_advance,
                                              &width_context) ==
                                *measured + static_cast<int>(name.size()) * advance('A', nullptr),
                            "Weapon name begins after the complete pickup prefix");
                }
            }
        }
        int checked{};
        for (unsigned code = 1; code <= 23; ++code) {
            const auto record = icons.inline_text(std::string(1, static_cast<char>(code)));
            if (record.size() == 1)
                continue;
            require(record.size() == 14 && inline_material_bytes(record, 0) == 14 &&
                        record.find('\0') == std::string::npos,
                    "Atomic native button record");
            const auto name = record.substr(5);
            auto* material = static_cast<std::byte*>(icons.material(name));
            require(material != nullptr, "Every button resolves to its owned material");
            const std::byte* table{};
            const std::byte* image{};
            IDirect3DTexture9* texture{};
            std::memcpy(&table, material + 0x68, sizeof(table));
            std::memcpy(&image, table + 8, sizeof(image));
            std::memcpy(&texture, image, sizeof(IDirect3DTexture9*));
            D3DSURFACE_DESC desc{};
            require(SUCCEEDED(texture->GetLevelDesc(0, &desc)), "Real button texture readable");
            require(desc.Width <= 60 && desc.Height <= 60 && desc.Format == D3DFMT_A8R8G8B8,
                    "Only button pixels uploaded");
            if (code == 14 || code == 15)
                require(desc.Width == 32 && desc.Height == (ps3 ? 20u : 32u),
                        "Start/Select preserve selected platform dimensions");
            if (code == 16 || code == 17)
                require(desc.Width == (ps3 ? 58u : 54u) && desc.Height == 33,
                        "Stick icons use selected platform resource");
            D3DLOCKED_RECT locked{};
            require(SUCCEEDED(texture->LockRect(0, &locked, nullptr, D3DLOCK_READONLY)),
                    "Button texture can be inspected");
            unsigned visible{}, blue{}, pink{};
            for (unsigned y = 0; y < desc.Height; ++y)
                for (unsigned x = 0; x < desc.Width; ++x) {
                    const auto* pixel =
                        static_cast<const unsigned char*>(locked.pBits) + y * locked.Pitch + x * 4;
                    if (pixel[3] > 64) {
                        ++visible;
                        if (pixel[0] > pixel[1] + 20 && pixel[0] > pixel[2] + 20)
                            ++blue;
                        if (pixel[0] > pixel[1] + 20 && pixel[2] > pixel[1] + 20)
                            ++pink;
                    }
                }
            texture->UnlockRect(0);
            require(visible > 0, "Icons retain alpha");
            if (code == (ps3 ? 1u : 3u))
                require(blue > 0, "Cross/X retains original blue pixels");
            if (ps3 && code == 3)
                require(pink > 0, "PS3 Square retains original pink pixels");
            for (int height : {12, 24, 33, 47, 64, 96}) {
                font.pixel_height = height;
                const auto metric = icons.button_glyph(&font, code);
                require(metric &&
                            metric->advance == image_dimension(static_cast<unsigned char>(record[2]), height),
                        "Glyph-width adapter agrees with renderer at every tested font height");
                const auto width = prompt_text_width(record, 1, height, read_utf8, advance, nullptr);
                require(width && *width == metric->advance,
                        "Material name bytes don't consume maxChars or width");
            }
            ++checked;
        }
        require(checked == 16, "Exactly 16 independent buttons");
        const auto record = icons.inline_text("\x03");
        require(prompt_text_width("\xD0\x96" + record + "A", 1, 33, read_utf8, advance, nullptr) == 11,
                "UTF-8 code point counts once before icon cutoff");
        require(prompt_text_width("A\n" + record, 0, 33, read_utf8, advance, nullptr) ==
                    image_dimension(static_cast<unsigned char>(record[2]), 33),
                "Multiline maximum includes images");
        require(prompt_visible_characters("A\r\n" + record + "B", read_utf8, nullptr) == 3,
                "FX count spans lines and counts image as one item");
        require(!prompt_text_width(record.substr(0, 7), 0, 33, read_utf8, advance, nullptr),
                "Truncated material fails atomically");
        icons.release_gpu_resources();
        require(!icons.ready() && !icons.material("mw3gf/p03") && !icons.font(),
                "Renderer teardown removes all published GPU resources");
        require(icons.prepare(&font, ui_surface.material.data()), "Images recreate after renderer restart");
        icons.release_gpu_resources();
        DestroyWindow(window);
        window = nullptr;
        FreeLibrary(module);
        module = nullptr;
        std::cout << (ps3 ? "PS3" : "X360")
                  << " 16 icons: D3D9 uploads, platform metrics/colors, foreign font preservation, UTF-8 "
                     "widths, cutoff, "
                     "weapon records and GPU restart verified\n";
        return 0;
    } catch (const std::exception& error) {
        if (window)
            DestroyWindow(window);
        if (module)
            FreeLibrary(module);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
