#include "mw3gf/core/status_text.hpp"
namespace mw3gf {
std::wstring pressed_controls(const InputSnapshot& snapshot) {
    if (!snapshot.connected)
        return L"no controller";
    std::wstring text;
    auto append = [&text](std::wstring_view name) {
        if (!text.empty())
            text += L" + ";
        text += name;
    };
    for (const auto& [button, name] : button_labels)
        if (snapshot.pad.held(button))
            append(name);
    if (snapshot.pad.left_trigger > 0.12f)
        append(L"LT");
    if (snapshot.pad.right_trigger > 0.12f)
        append(L"RT");
    constexpr float threshold = 0.3f; // Display threshold only; no alteration to raw input.
    const auto& left = snapshot.pad.left_stick;
    const auto& right = snapshot.pad.right_stick;
    if (left.x < -threshold)
        append(L"LS_LEFT");
    if (left.x > threshold)
        append(L"LS_RIGHT");
    if (left.y < -threshold)
        append(L"LS_DOWN");
    if (left.y > threshold)
        append(L"LS_UP");
    if (right.x < -threshold)
        append(L"RS_LEFT");
    if (right.x > threshold)
        append(L"RS_RIGHT");
    if (right.y < -threshold)
        append(L"RS_DOWN");
    if (right.y > threshold)
        append(L"RS_UP");
    return text.empty() ? L"none" : text;
}
} // namespace mw3gf
