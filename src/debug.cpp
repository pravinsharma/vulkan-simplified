#include "vks/debug.hpp"

#include <algorithm>
#include <mutex>

namespace vks {

DebugLayer& DebugLayer::instance() {
    static DebugLayer instance;
    return instance;
}

void DebugLayer::enableValidationLayers(bool enable) {
    enabled_ = enable;
}

bool DebugLayer::isEnabled() const {
    return enabled_;
}

void DebugLayer::pushWarning(std::string message) {
    if (!enabled_) return;
    std::lock_guard<std::mutex> lock(mutex_);
    diagnostics_.warnings.push_back(std::move(message));
}

void DebugLayer::setFrameDiagnostics(FrameDiagnostics diagnostics) {
    std::lock_guard<std::mutex> lock(mutex_);
    diagnostics_ = std::move(diagnostics);
}

const FrameDiagnostics& DebugLayer::lastFrameDiagnostics() const {
    return diagnostics_;
}

void DebugLayer::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    diagnostics_ = FrameDiagnostics{};
}

}
