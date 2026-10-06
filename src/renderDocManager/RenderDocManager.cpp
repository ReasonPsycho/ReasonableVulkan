#include "RenderDocManager.hpp"
#include <windows.h>
#include <spdlog/spdlog.h>

namespace rd {

RenderDocManager::RenderDocManager() {
    if (HMODULE mod = GetModuleHandleA("renderdoc.dll")) {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
        if (RENDERDOC_GetAPI) {
            int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&rdocApi);
            if (ret != 1) {
                spdlog::error("Failed to initialize RenderDoc API");
                rdocApi = nullptr;
            } else {
                spdlog::info("RenderDoc API initialized successfully");
            }
        }
    } else {
        spdlog::warn("RenderDoc DLL not found. Manual captures will be disabled.");
    }
}

RenderDocManager::~RenderDocManager() {
}

void RenderDocManager::triggerCapture() {
    if (rdocApi) {
        rdocApi->TriggerCapture();
        spdlog::info("RenderDoc: TriggerCapture requested");
    }
}

void RenderDocManager::requestCapture(uint32_t numFrames, bool autoOpenUI) {
    if (!rdocApi) {
        spdlog::warn("Cannot request RenderDoc capture: RenderDoc API is not available");
        return;
    }
    captureFramesRemaining = numFrames;
    openUiOnFinish = autoOpenUI;
    spdlog::info("RenderDoc: Capture requested for {} frame(s)", numFrames);
}

void RenderDocManager::startCapture(void* deviceHandle) {
    if (rdocApi && !capturing) {
        rdocApi->StartFrameCapture(deviceHandle, NULL);
        capturing = true;
    }
}

void RenderDocManager::endCapture(void* deviceHandle) {
    if (rdocApi && capturing) {
        rdocApi->EndFrameCapture(deviceHandle, NULL);
        capturing = false;
    }
}

void RenderDocManager::onFrameBegin(void* deviceHandle) {
    if (!rdocApi) return;
    if (captureFramesRemaining > 0 && !capturing) {
        rdocApi->StartFrameCapture(deviceHandle, NULL);
        capturing = true;
        spdlog::info("RenderDoc: Started frame capture");
    }
}

void RenderDocManager::onFrameEnd(void* deviceHandle) {
    if (!rdocApi || !capturing) return;

    rdocApi->EndFrameCapture(deviceHandle, NULL);
    capturing = false;
    if (captureFramesRemaining > 0) {
        captureFramesRemaining--;
    }
    spdlog::info("RenderDoc: Ended frame capture (remaining frames: {})", captureFramesRemaining);

    if (openUiOnFinish && captureFramesRemaining == 0) {
        launchReplayUI();
    }
}

void RenderDocManager::launchReplayUI() {
    if (rdocApi) {
        if (!rdocApi->IsTargetControlConnected()) {
            rdocApi->LaunchReplayUI(1, nullptr);
        }
    }
}

} // namespace rd
