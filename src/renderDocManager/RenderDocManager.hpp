#pragma once

#include "renderdoc_app.h"
#include <cstdint>

namespace rd {

class RenderDocManager {
public:
    RenderDocManager();
    ~RenderDocManager();

    bool isApiAvailable() const { return rdocApi != nullptr; }
    bool isCapturing() const { return capturing; }
    bool isCaptureRequested() const { return captureFramesRemaining > 0; }

    void triggerCapture();
    void requestCapture(uint32_t numFrames = 1, bool autoOpenUI = false);

    void startCapture(void* deviceHandle = nullptr);
    void endCapture(void* deviceHandle = nullptr);

    void onFrameBegin(void* deviceHandle = nullptr);
    void onFrameEnd(void* deviceHandle = nullptr);

    void launchReplayUI();

    static RenderDocManager& getInstance() {
        static RenderDocManager instance;
        return instance;
    }

private:
    RENDERDOC_API_1_1_2* rdocApi = nullptr;
    uint32_t captureFramesRemaining = 0;
    bool capturing = false;
    bool openUiOnFinish = false;
};

} // namespace rd
