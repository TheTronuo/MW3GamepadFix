#include "mw3gf/core/stick_camera.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("Expected Xbox turn fixture");
        std::ifstream file(argv[1]);
        if (!file) throw std::runtime_error("Cannot open Xbox turn fixture");
        mw3gf::StickCamera camera;
        std::string line;
        unsigned count{};
        while (std::getline(file, line)) {
            if (line == "reset") { camera.reset(); continue; }
            mw3gf::Stick stick;
            mw3gf::CameraContext context;
            mw3gf::CameraSettings settings;
            float pitch{}, yaw{};
            std::istringstream row(line);
            if (!(row >> stick.x >> stick.y >> context.seconds >> context.ads >> context.fov_scale
                      >> context.pitch_cap >> context.yaw_cap >> context.pitch_scale >> context.yaw_scale
                      >> settings.sensitivity >> pitch >> yaw))
                throw std::runtime_error("Malformed Xbox turn row");
            const auto actual = camera.update(stick, context, settings);
            if (std::abs(actual.pitch - pitch) > .0002f || std::abs(actual.yaw - yaw) > .0002f) {
                std::cerr << "Frame " << count << ": PC " << actual.pitch << ',' << actual.yaw
                          << "; Xbox " << pitch << ',' << yaw << '\n';
                throw std::runtime_error("Original Xbox turn mismatch");
            }
            ++count;
        }
        if (count != 2400) throw std::runtime_error("Incomplete Xbox turn fixture");
        std::cout << count << " original Xbox curve/turn frames verified\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
