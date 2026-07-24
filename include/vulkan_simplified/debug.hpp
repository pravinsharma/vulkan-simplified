#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace vks {

struct FrameDiagnostics {
    uint32_t drawCalls = 0;
    uint32_t triangleCount = 0;
    uint32_t pipelineCount = 0;
    std::vector<std::string> warnings;
};

class DebugLayer {
public:
    static DebugLayer& instance();

    void enableValidationLayers(bool enable);
    bool isEnabled() const;

    void pushWarning(std::string message);
    void setFrameDiagnostics(FrameDiagnostics diagnostics);
    const FrameDiagnostics& lastFrameDiagnostics() const;
    void reset();

private:
    DebugLayer() = default;
    bool enabled_ = false;
    mutable std::mutex mutex_;
    FrameDiagnostics diagnostics_;
};

}
