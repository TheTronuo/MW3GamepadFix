#include "mw3gf/game/input_session.hpp"
#include <stdexcept>
namespace mw3gf::game {
InputSession::InputSession(const plugin::StartOptions& options, RuntimeLog& log) : demo_(options.demo != 0) {
    if (!demo_) {
        if (options.backend == plugin::BackendSelection::automatic) {
            try {
                backend_ = make_backend(BackendKind::gameinput);
            } catch (const std::exception& error) {
                log.write(error.what());
                log.write("Automatic selection: XInput 1.4 fallback.");
                backend_ = make_backend(BackendKind::xinput);
            }
        } else {
            backend_ =
                make_backend(options.backend == plugin::BackendSelection::gameinput ? BackendKind::gameinput
                                                                                    : BackendKind::xinput);
        }
    }
    log.write(demo_ ? "DEMO: synthetic data, not a connected controller." : "Input adapter ready.");
}
void InputSession::poll_gameplay() {
    if (!demo_)
        monitor_.update(backend_->poll());
}
void InputSession::poll_menu() {
    InputSnapshot snapshot;
    if (demo_) {
        snapshot.connected = true;
        snapshot.backend = L"DEMO";
        snapshot.device_id = L"synthetic";
        snapshot.device_name = L"Synthetic input";
    } else
        snapshot = backend_->poll();
    monitor_.update(std::move(snapshot));
}
} // namespace mw3gf::game
