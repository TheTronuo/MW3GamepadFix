#include "mw3gf/core/stick_camera.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace {
void require(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
bool near(float a, float b, float error = .001f) {
    return std::abs(a - b) <= error;
}
float turn(int fps) {
    mw3gf::StickCamera camera;
    float angle{};
    for (int i = 0; i < fps; ++i)
        angle += camera.update({1, 0}, {1.0f / fps}, {}).yaw;
    return angle;
}
} // namespace
int main() {
    try {
        using namespace mw3gf;
        require(xbox_stick({.1f, .1f}).x == 0 && xbox_stick({0, -.2f}).y == 0,
                "Radial deadzone at and inside threshold");
        require(xbox_stick({0, 1}).y == 1 && xbox_stick({-1, 0}).x == -1,
                "Outer deadzone saturates endpoints");
        const auto partial = xbox_stick({0, .6f});
        require(near(partial.y, (.6f * 32767 / 32768 - .2f) / .79f, .00002f),
                "Xbox positive hardware normalization and axis quantization");
        const auto walk = xbox_movement(partial);
        require(walk.y == 32, "Squared walk speed, not linear movement");
        const auto diagonal = xbox_movement(xbox_stick({1, 1}));
        require(diagonal.x >= 126 && diagonal.y >= 126,
                "Native diagonal correction, with signed-byte truncation");
        const auto curve = xbox_view_curve({.5116f, 0});
        require(near(curve.x, .5116f * .3508f, .00001f),
                "Original graph 3 supplies a multiplier, not output magnitude");
        const auto cross = xbox_view_curve({.8f, .1f});
        require(cross.y / cross.x < .125f && cross.x > 0, "Minor-axis damping does not affect major axis");
        StickCamera camera;
        auto first = camera.update({1, 0}, {.01f}, {});
        require(near(first.yaw, -.12f), "Xbox acceleration 1200 deg/s squared");
        CameraDelta last;
        for (int i = 0; i < 30; ++i)
            last = camera.update({1, 1}, {.01f}, {});
        require(near(last.yaw, -2.6f) && near(last.pitch, -.9f), "Hip yaw/pitch native rates and signs");
        last = camera.update({-1, -1}, {.01f}, {});
        require(near(last.yaw, 2.6f) && near(last.pitch, .9f),
                "Reversal uses current sign without artificial brake");
        last = camera.update({}, {.01f}, {});
        require(last.yaw == 0 && last.pitch == 0, "Release has no drift or easing tail");
        first = camera.update({1, 0}, {.01f}, {});
        require(near(first.yaw, -.12f), "Release clears velocity before a new turn");
        camera.reset();
        for (int i = 0; i < 30; ++i)
            last = camera.update({1, 1}, {.01f, 1, .5f}, {});
        require(near(last.yaw, -.45f) && near(last.pitch, -.275f), "Full ADS and real FOV scale");
        for (int i = 0; i < 30; ++i)
            last = camera.update({1, 1}, {.01f, 0, 1, 10, 20}, {4, true});
        require(near(last.yaw, -.2f) && near(last.pitch, .1f), "Native turn caps and inverted pitch");
        const float a30 = turn(30), a60 = turn(60), a120 = turn(120);
        require(a30 < -230 && a120 < -230 && std::abs(a30 - a120) < 4 && std::abs(a60 - a120) < 2,
                "30/60/120 FPS use game seconds; only original discrete acceleration integration differs");
        const float nan = std::numeric_limits<float>::quiet_NaN();
        require(xbox_stick({nan, 1}).y == 0, "Invalid stick packet is neutral");
        last = camera.update({nan, 0}, {.01f}, {});
        require(last.yaw == 0 && last.pitch == 0, "Invalid input resets camera safely");
        last = camera.update({1, 0}, {0}, {});
        require(last.yaw == 0, "No time step means no rotation");
        first = camera.update({1, 0}, {.01f}, {});
        require(near(first.yaw, -.12f), "Invalid/zero frame resets old acceleration");
        std::cout << "Xbox sticks, curve, acceleration, frame timing, ADS/FOV/caps and reset verified\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
