#pragma once
#include "mw3gf/core/pad_state.hpp"
#include <memory>

namespace mw3gf {
enum class BackendKind { xinput, gameinput };
class InputBackend {
  public:
    virtual ~InputBackend() = default;
    [[nodiscard]] virtual InputSnapshot poll() = 0;
};
// Construction reports unavailable runtimes with an exception; no silent API downgrade.
[[nodiscard]] std::unique_ptr<InputBackend> make_backend(BackendKind kind);
[[nodiscard]] std::unique_ptr<InputBackend> make_xinput_backend();
[[nodiscard]] std::unique_ptr<InputBackend> make_gameinput_backend();
} // namespace mw3gf
