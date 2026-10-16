#include "mw3gf/core/console_aim.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
using namespace mw3gf;
namespace {
void require(bool value, const std::string& message) {
    if (!value) throw std::runtime_error(message);
}
bool near(float a, float b, float tolerance = .0002f) {
    return std::isfinite(a) && std::abs(a - b) <= tolerance;
}
void oracle(const char* path) {
    std::ifstream file(path);
    require(file.good(), "Missing original Xbox results");
    std::string line;
    int count{};
    ConsoleAim ads_aim;
    while (std::getline(file, line)) {
        std::istringstream row(line);
        std::string kind;
        row >> kind;
        AimTarget target;
        target.entity = 7;
        AimFrame frame;
        frame.targets = {&target, 1};
        frame.hip_range = 100; frame.ads_range = 200;
        frame.slowdown_region = frame.lock_region = {.2f, .2f};
        float box{}, pitch{}, yaw{};
        if (kind == "slow") {
            row >> frame.flags >> frame.movement_flags >> frame.weapon_state >> frame.weapon_valid
                >> frame.has_ammo >> frame.ads >> frame.scripted_slowdown >> frame.pitch_scale
                >> frame.yaw_scale >> frame.skip_pitch >> target.distance_squared >> box >> pitch >> yaw;
            target.minimum = {box, box}; target.maximum = {box + .2f, box + .2f};
            const auto value = console_slowdown(frame);
            require(near(value[0], pitch) && near(value[1], yaw), "Slowdown mismatch: " + line);
        } else if (kind == "lock") {
            Stick look{}, move{};
            row >> frame.movement_flags >> frame.melee_phase >> frame.ads >> frame.weapon_valid
                >> look.x >> look.y >> move.x >> frame.skip_pitch >> target.distance_squared
                >> box >> frame.lock_seconds >> pitch >> yaw;
            frame.velocity = {0, 2, 3}; frame.right = {0, 1, 0}; frame.up = {0, 0, 1};
            target.velocity = {0, 10, 20};
            target.minimum = {box, box}; target.maximum = {box + .2f, box + .2f};
            ConsoleAim aim;
            const auto value = aim.lock_on(frame, look, move);
            require(near(value.pitch, pitch) && near(value.yaw, yaw), "Lock mismatch: " + line);
        } else if (kind == "track") {
            float goal{}, current{}, seconds{}, expected{};
            row >> goal >> current >> seconds >> expected;
            require(near(console_track_angle(goal, current, seconds), expected), "Angle tracking mismatch: " + line);
        } else if (kind == "cone") {
            ViewCone cone;
            float input{}, expected{};
            row >> cone.constrained >> cone.pitch >> cone.angle >> cone.center >> cone.range
                >> cone.positive_zone >> cone.negative_zone >> input >> expected;
            // The fixture records native axis: zero is pitch, one is yaw.
            cone.pitch = !cone.pitch;
            require(near(console_view_resistance(cone, input), expected), "View cone mismatch: " + line);
        } else if (kind == "reset") {
            ads_aim.reset();
        } else if (kind == "auto") {
            bool present{}, active{};
            row >> frame.buttons >> frame.ads >> frame.flags >> frame.movement_flags >> frame.weapon_valid
                >> frame.offhand >> frame.weapon_state >> frame.weapon_type >> present >> frame.enabled
                >> frame.seconds >> target.position[0] >> target.position[1] >> target.position[2]
                >> frame.view_angles[0] >> frame.view_angles[1] >> pitch >> yaw >> active;
            frame.auto_range = 200; frame.auto_region = {.2f, .2f};
            target.minimum = {-.1f, -.1f}; target.maximum = {.1f, .1f};
            target.distance_squared = 10000;
            if (!present) frame.targets = {};
            const auto value = ads_aim.auto_aim(frame);
            require(near(value.pitch, pitch) && near(value.yaw, yaw) && ads_aim.auto_aim_active() == active,
                    "ADS mismatch: " + line);
        } else throw std::runtime_error("Unknown Xbox fixture row");
        require(!row.fail(), "Incomplete Xbox fixture row");
        ++count;
    }
    require(count == 1630, "Incomplete Xbox fixture");
}
void transitions() {
    AimTarget targets[2];
    targets[0].entity = 3; targets[0].distance_squared = 100;
    targets[0].position = {100, 20, 10};
    targets[0].minimum = {-.1f, -.1f}; targets[0].maximum = {.1f, .1f};
    targets[1] = targets[0]; targets[1].entity = 4;
    AimFrame frame;
    frame.targets = targets; frame.weapon_valid = frame.has_ammo = true;
    frame.auto_range = frame.hip_range = frame.ads_range = 200;
    frame.auto_region = frame.lock_region = {.2f, .2f};
    frame.seconds = frame.lock_seconds = 1.0f / 60;
    frame.buttons = 0x800;
    ConsoleAim aim;
    auto result = aim.auto_aim(frame);
    require(result.pitch < 0 && result.yaw > 0 && aim.auto_aim_active(), "ADS rising edge acquires target");
    frame.targets = {};
    result = aim.auto_aim(frame);
    require(result.pitch == 0 && result.yaw == 0 && !aim.auto_aim_active(), "Lost target ends ADS assistance");
    frame.targets = targets;
    result = aim.auto_aim(frame);
    require(result.pitch == 0 && result.yaw == 0, "Holding ADS cannot repeatedly snap to new targets");
    frame.buttons = 0; (void)aim.auto_aim(frame);
    frame.buttons = 0x800;
    result = aim.auto_aim(frame);
    require(result.yaw > 0, "Releasing ADS allows a fresh acquisition");
    aim.reset();
    result = aim.auto_aim(frame);
    require(result.yaw > 0, "Disconnect/mode reset clears target and ADS edge");
    frame.enabled = false;
    result = aim.auto_aim(frame);
    require(result.yaw == 0 && !aim.auto_aim_active(), "Assist setting resets active snap");
    frame.scripted_slowdown = true; frame.pitch_scale = .3f; frame.yaw_scale = .7f;
    require(console_slowdown(frame)[1] == .7f, "Assist setting retains scripted camera coefficients");
    require(console_target(targets, 200, {.2f, .2f}, 4)->entity == 4, "Previous lock target has priority");
    targets[1].distance_squared = 40000;
    require(console_target(targets, 200, {.2f, .2f}, 4)->entity == 3, "Previous target uses strict range boundary");
    frame.enabled = true;
    frame.velocity = {}; frame.right = {0, 1, 0}; frame.up = {0, 0, 1};
    targets[0].velocity = {0, 10, 0};
    require(aim.lock_on(frame, {}, {0, 1}).yaw == 0, "Forward axis alone does not engage lock-on");
    require(aim.lock_on(frame, {}, {.05f, 0}).yaw > 0, "Side threshold engages lock-on");
    frame.melee_phase = 2;
    require(aim.lock_on(frame, {1, 1}, {1, 1}).yaw == 0, "Melee charge blocks lock-on");
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Xbox fixture path required");
        oracle(argv[1]); transitions();
        std::cout << "1600 original Xbox PPC results, 30 ADS sequences and lock/reset transitions verified\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n'; return 1;
    }
}
