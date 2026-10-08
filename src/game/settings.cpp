#include "mw3gf/game/settings.hpp"
#include <fstream>
#include <stdexcept>
#include <string_view>

namespace mw3gf::game {
namespace {
std::string_view trim(std::string_view value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string_view::npos)
        return {};
    return value.substr(begin, value.find_last_not_of(" \t\r\n") - begin + 1);
}
} // namespace

ModSettings read_settings(const std::filesystem::path& path) {
    ModSettings result;
    std::ifstream input(path);
    std::string line;
    bool input_section{};
    while (std::getline(input, line)) {
        if (line.starts_with("\xEF\xBB\xBF"))
            line.erase(0, 3);
        const auto text = trim(std::string_view(line).substr(0, line.find_first_of(";#")));
        if (text.empty())
            continue;
        if (text.front() == '[') {
            input_section = text == "[Input]";
            continue;
        }
        if (!input_section)
            continue;
        const auto equals = text.find('=');
        if (equals == std::string_view::npos)
            continue;
        const auto key = trim(text.substr(0, equals));
        const auto value = trim(text.substr(equals + 1));
        if (key == "enabled") {
            if (value != "0" && value != "1")
                throw std::runtime_error("Input enabled must be 0 or 1");
            result.enabled = value == "1";
        } else if (key == "backend") {
            if (value == "xinput")
                result.backend = plugin::BackendSelection::xinput;
            else if (value == "gameinput")
                result.backend = plugin::BackendSelection::gameinput;
            else if (value == "auto")
                result.backend = plugin::BackendSelection::automatic;
            else
                throw std::runtime_error("Input backend must be xinput, gameinput or auto");
        } else if (key == "prompts") {
            if (value == "x360")
                result.prompt_style = PromptStyle::x360;
            else if (value == "ps3")
                result.prompt_style = PromptStyle::ps3;
            else
                throw std::runtime_error("Input prompts must be x360 or ps3");
        }
    }
    return result;
}
} // namespace mw3gf::game
