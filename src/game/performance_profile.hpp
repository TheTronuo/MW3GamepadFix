#pragma once
#include "mw3gf/game/runtime_log.hpp"
// Opt-in diagnostic build only. Timings include nested hooks and native calls;
// do not add the rows together. The normal build compiles these scopes away.
#ifdef MW3GF_PROFILE
#include <Windows.h>
#include <array>
#include <atomic>
#include <cstdio>
namespace mw3gf::game::detail {
enum class ProfilePoint {
    input, menu_input, native_frame, menu_paint, item_paint, focusable, camera,
    aim, remote, localized, text_submit, glyph, handle_pic, text_render,
    text_width, decoded_width, text_count, material, binding, count
};
struct ProfileCounter {
    std::atomic_ullong ticks{}, calls{}, maximum{};
};
inline std::array<ProfileCounter, static_cast<std::size_t>(ProfilePoint::count)> profile_counters;
inline unsigned long long profile_now() noexcept {
    LARGE_INTEGER value{};
    QueryPerformanceCounter(&value);
    return static_cast<unsigned long long>(value.QuadPart);
}
class ProfileScope {
  public:
    explicit ProfileScope(ProfilePoint point) noexcept
        : counter_(profile_counters[static_cast<std::size_t>(point)]), start_(profile_now()) {}
    ~ProfileScope() {
        const auto elapsed = profile_now() - start_;
        counter_.ticks.fetch_add(elapsed, std::memory_order_relaxed);
        counter_.calls.fetch_add(1, std::memory_order_relaxed);
        auto maximum = counter_.maximum.load(std::memory_order_relaxed);
        while (maximum < elapsed && !counter_.maximum.compare_exchange_weak(
                   maximum, elapsed, std::memory_order_relaxed)) {}
    }
  private:
    ProfileCounter& counter_;
    unsigned long long start_;
};
inline void profile_report(RuntimeLog& log, bool connected) noexcept {
    static const auto frequency = [] {
        LARGE_INTEGER value{};
        QueryPerformanceFrequency(&value);
        return static_cast<unsigned long long>(value.QuadPart);
    }();
    static auto previous = profile_now();
    static unsigned long long frames{};
    ++frames;
    const auto now = profile_now();
    if (now - previous < frequency * 5) return;
    const auto frame_count = frames;
    const double seconds = static_cast<double>(now - previous) / frequency;
    previous = now;
    frames = 0;
    constexpr std::array names{"input", "menu_input", "native_frame", "menu_paint", "item_paint",
        "focusable", "camera", "aim", "remote", "localized", "text_submit", "glyph", "handle_pic",
        "text_render", "text_width", "decoded_width", "text_count", "material", "binding"};
    char line[256]{};
    std::snprintf(line, sizeof(line), "PERF pad=%d client_frames/s=%.1f window=%.2fs (inclusive timings)",
                  connected ? 1 : 0, frame_count / seconds, seconds);
    log.write(line);
    for (std::size_t i = 0; i < profile_counters.size(); ++i) {
        auto& counter = profile_counters[i];
        const auto ticks = counter.ticks.exchange(0, std::memory_order_relaxed);
        const auto calls = counter.calls.exchange(0, std::memory_order_relaxed);
        const auto maximum = counter.maximum.exchange(0, std::memory_order_relaxed);
        if (!calls) continue;
        std::snprintf(line, sizeof(line), "PERF %s calls/frame=%.2f ms/frame=%.4f max_call_ms=%.4f",
            names[i], static_cast<double>(calls) / frame_count,
            static_cast<double>(ticks) * 1000 / frequency / frame_count,
            static_cast<double>(maximum) * 1000 / frequency);
        log.write(line);
    }
}
} // namespace mw3gf::game::detail
#define MW3GF_TIME(point) ProfileScope profile_scope{ProfilePoint::point}
#else
#define MW3GF_TIME(point) ((void)0)
namespace mw3gf::game::detail {
inline void profile_report(RuntimeLog&, bool) noexcept {}
}
#endif
