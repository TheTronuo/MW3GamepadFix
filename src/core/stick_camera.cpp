#include "mw3gf/core/stick_camera.hpp"
#include <algorithm>
#include <array>
#include <cmath>
namespace mw3gf {
namespace {
float axis(float value) noexcept {
    value = std::clamp(value, -1.0f, 1.0f);
    return value > 0 ? value * (32767.0f / 32768.0f) : value;
}
float stored(float value) noexcept {
    return std::trunc(value * 65535.0f) / 65535.0f;
}
float graph(float radius) noexcept {
    constexpr std::array<Stick, 7> points{{{0, 0},
                                           {.1242f, .0423f},
                                           {.2238f, .0885f},
                                           {.3210f, .1553f},
                                           {.4118f, .2419f},
                                           {.5116f, .3508f},
                                           {1, 1}}};
    radius = std::clamp(radius, 0.0f, 1.0f);
    for (std::size_t i = 1; i < points.size(); ++i)
        if (radius <= points[i].x) {
            const auto a = points[i - 1], b = points[i];
            return a.y + (b.y - a.y) * (radius - a.x) / (b.x - a.x);
        }
    return 1;
}
float speed(float input, float rate, float step, float& previous) noexcept {
    const float target = std::abs(input) * rate;
    previous = target > previous ? std::min(target, previous + step) : target;
    return std::copysign(previous, input);
}
} // namespace
Stick xbox_stick(Stick raw) noexcept {
    if (!std::isfinite(raw.x) || !std::isfinite(raw.y))
        return {};
    const float x = axis(raw.x), y = axis(raw.y), radius = std::hypot(x, y);
    if (radius <= .20f)
        return {};
    const float magnitude = std::clamp((radius - .20f) / .79f, 0.0f, 1.0f);
    return {stored(x / radius * magnitude), stored(y / radius * magnitude)};
}
Stick xbox_movement(Stick processed) noexcept {
    const float radius = std::hypot(processed.x, processed.y);
    const float major = std::max(std::abs(processed.x), std::abs(processed.y));
    if (!major)
        return {};
    const float minor = std::min(std::abs(processed.x), std::abs(processed.y));
    const float ratio = minor / major;
    const float scale = 127.0f * std::sqrt(1 + ratio * ratio) * radius;
    return {std::clamp(std::trunc(processed.x * scale), -127.0f, 127.0f),
            std::clamp(std::trunc(processed.y * scale), -127.0f, 127.0f)};
}
Stick xbox_view_curve(Stick processed) noexcept {
    const float coefficient = graph(std::hypot(processed.x, processed.y));
    Stick result{processed.x * coefficient, processed.y * coefficient};
    const float x = std::abs(result.x), y = std::abs(result.y);
    if (x > y)
        result.y *= 1 - (x - y);
    else
        result.x *= 1 - (y - x);
    return result;
}
CameraDelta StickCamera::update(Stick processed, const CameraContext& context,
                                const CameraSettings& settings) noexcept {
    if (!std::isfinite(processed.x) || !std::isfinite(processed.y) || !std::isfinite(context.seconds) ||
        context.seconds <= 0 || context.seconds > 1 || !std::isfinite(settings.sensitivity) ||
        settings.sensitivity < .1f || settings.sensitivity > 4 || !std::isfinite(context.ads) ||
        !std::isfinite(context.fov_scale) || context.fov_scale <= 0) {
        reset();
        return {};
    }
    const auto input = xbox_view_curve(processed);
    const float ads = std::clamp(context.ads, 0.0f, 1.0f);
    float pitch_rate = (90 - 35 * ads) * context.fov_scale * settings.sensitivity;
    float yaw_rate = (260 - 170 * ads) * context.fov_scale * settings.sensitivity;
    if (std::isfinite(context.pitch_cap) && context.pitch_cap > 0)
        pitch_rate = std::min(pitch_rate, context.pitch_cap);
    if (std::isfinite(context.yaw_cap) && context.yaw_cap > 0)
        yaw_rate = std::min(yaw_rate, context.yaw_cap);
    const float step = 1200 * settings.sensitivity * context.seconds;
    return {speed(settings.invert_y ? input.y : -input.y, pitch_rate, step, pitch_speed_) * context.seconds,
            speed(-input.x, yaw_rate, step, yaw_speed_) * context.seconds};
}
} // namespace mw3gf
