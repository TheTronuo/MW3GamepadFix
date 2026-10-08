#include "mw3gf/game/settings.hpp"
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
struct ConfigurationFile {
    std::filesystem::path path;
    ConfigurationFile() {
        wchar_t folder[MAX_PATH]{}, name[MAX_PATH]{};
        require(GetTempPathW(MAX_PATH, folder) != 0 && GetTempFileNameW(folder, L"m3g", 0, name) != 0,
                "Cannot create temporary configuration file");
        path = name;
    }
    ~ConfigurationFile() {
        std::error_code error;
        std::filesystem::remove(path, error);
    }
    void write(const char* text) { std::ofstream(path, std::ios::binary) << text; }
};
} // namespace
int main() {
    using namespace mw3gf::game;
    using mw3gf::PromptStyle;
    try {
        ConfigurationFile file;
        const auto defaults = read_settings(file.path);
        require(defaults.navigation && defaults.gameplay && defaults.camera && defaults.code_page == 1252,
                "Default settings changed");
        require(defaults.enabled && defaults.backend == mw3gf::plugin::BackendSelection::xinput &&
                    defaults.prompt_style == PromptStyle::x360,
                "Default input configuration changed");
        file.write("\xEF\xBB\xBF[Input]\r\n enabled = 0 \r\n backend = gameinput ; comment\r\n"
                   "prompts = ps3\r\n");
        const auto settings = read_settings(file.path);
        require(!settings.enabled && settings.backend == mw3gf::plugin::BackendSelection::gameinput &&
                    settings.prompt_style == PromptStyle::ps3,
                "Three public settings parse with BOM, whitespace and comments");
        file.write("[Input]\nbackend=auto\nprompts=x360\nenabled=1\n"
                   "[Camera]\nsensitivity=bad\ninvert_y=1\n"
                   "[Menu]\nnavigation=0\n[Gameplay]\nbuttons=0\n");
        const auto automatic = read_settings(file.path);
        require(automatic.backend == mw3gf::plugin::BackendSelection::automatic && automatic.enabled &&
                    automatic.prompt_style == PromptStyle::x360,
                "Automatic backend and X360 selection parse");
        require(automatic.navigation && automatic.gameplay && automatic.camera &&
                    automatic.camera_settings.sensitivity == 1 && !automatic.camera_settings.invert_y,
                "Removed configuration sections cannot change runtime defaults");
        for (const char* invalid : {"[Input]\nenabled=2\n", "[Input]\nbackend=unknown\n",
                                    "[Input]\nprompts=ps4\n", "[Input]\nprompts=\n"}) {
            file.write(invalid);
            bool rejected{};
            try {
                (void)read_settings(file.path);
            } catch (const std::runtime_error&) {
                rejected = true;
            }
            require(rejected, "Invalid public setting silently accepted");
        }
        std::cout << "Three-key INI configuration verified\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
