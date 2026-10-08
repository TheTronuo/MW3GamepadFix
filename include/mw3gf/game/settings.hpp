#pragma once
#include "mw3gf/core/prompt_style.hpp"
#include "mw3gf/core/stick_camera.hpp"
#include "mw3gf/plugin/api.hpp"
#include <filesystem>
#include <string>
namespace mw3gf::game {
struct ModSettings {
    bool enabled{true};
    plugin::BackendSelection backend{plugin::BackendSelection::xinput};
    PromptStyle prompt_style{PromptStyle::x360};
    // Internal defaults; only input backend, activation and prompt style are configurable.
    int x_percent{4}, y_percent{93}, height_percent{3};
    unsigned code_page{1252};
    bool show_axes{}, show_debug{}, navigation{true}, prompts{true};
    bool row_prompts{true};
    bool gameplay{true}, gameplay_prompts{true};
    bool camera{true};
    CameraSettings camera_settings;
    std::wstring label{L"Pressed"};
    std::wstring change{L"Change"};
};

ModSettings read_settings(const std::filesystem::path& path);
} // namespace mw3gf::game
