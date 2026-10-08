#include "mw3gf/input/backend.hpp"
namespace mw3gf {
std::unique_ptr<InputBackend> make_backend(BackendKind kind) {
    return kind == BackendKind::gameinput ? make_gameinput_backend() : make_xinput_backend();
}
} // namespace mw3gf
