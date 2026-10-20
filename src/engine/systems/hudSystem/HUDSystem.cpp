#include "HUDSystem.hpp"
#include "ecs/Scene.h"
#include "Engine.h"
#include "PlatformInterface.hpp"
#include <algorithm>

namespace engine::ecs {

    HUDSystem::HUDSystem(Scene* scene) : System(scene) {}

    glm::vec2 HUDSystem::CalculateAnchoredPosition(UIAnchor anchor, const glm::vec2& offset, const glm::vec2& size, const glm::vec2& screenSize) const {
        glm::vec2 anchorPos(0.0f);

        switch (anchor) {
            case UIAnchor::TopLeft:      anchorPos = glm::vec2(0.0f, 0.0f); break;
            case UIAnchor::TopCenter:    anchorPos = glm::vec2(screenSize.x * 0.5f - size.x * 0.5f, 0.0f); break;
            case UIAnchor::TopRight:     anchorPos = glm::vec2(screenSize.x - size.x, 0.0f); break;
            case UIAnchor::CenterLeft:   anchorPos = glm::vec2(0.0f, screenSize.y * 0.5f - size.y * 0.5f); break;
            case UIAnchor::Center:       anchorPos = glm::vec2((screenSize.x - size.x) * 0.5f, (screenSize.y - size.y) * 0.5f); break;
            case UIAnchor::CenterRight:  anchorPos = glm::vec2(screenSize.x - size.x, screenSize.y * 0.5f - size.y * 0.5f); break;
            case UIAnchor::BottomLeft:   anchorPos = glm::vec2(0.0f, screenSize.y - size.y); break;
            case UIAnchor::BottomCenter: anchorPos = glm::vec2(screenSize.x * 0.5f - size.x * 0.5f, screenSize.y - size.y); break;
            case UIAnchor::BottomRight:  anchorPos = glm::vec2(screenSize.x - size.x, screenSize.y - size.y); break;
        }

        return anchorPos + offset;
    }

    void HUDSystem::Update(float deltaTime) {
        if (!scene || !scene->engine.graphicsEngine || scene->engine.minimized) {
            return;
        }

        int width = 0;
        int height = 0;
#ifdef ENABLE_IMGUI
        glm::uvec2 extent = scene->engine.graphicsEngine->getExtent();
        width = static_cast<int>(extent.x);
        height = static_cast<int>(extent.y);
#else
        if (scene->engine.platform) {
            scene->engine.platform->GetWindowSize(width, height);
        }
#endif
        if (width <= 0 || height <= 0) {
            return;
        }

        glm::vec2 screenSize(static_cast<float>(width), static_cast<float>(height));
        std::vector<gfx::HUDDrawItem> drawItems;

        for (auto [entity, hud] : scene->GetActiveView<HUDElementComponent>()) {
            if (!hud.isVisible) continue;

            gfx::HUDDrawItem item;
            UIAnchor anchor = static_cast<UIAnchor>(hud.anchor);
            item.screenPosition = CalculateAnchoredPosition(anchor, hud.offset, hud.size, screenSize);
            item.size = hud.size;
            item.color = hud.color;
            item.uvBounds = hud.uvBounds;
            item.textureAssetId = hud.textureUuid;
            item.layerDepth = hud.layerDepth;

            drawItems.push_back(item);
        }

        std::sort(drawItems.begin(), drawItems.end(), [](const gfx::HUDDrawItem& a, const gfx::HUDDrawItem& b) {
            return a.layerDepth < b.layerDepth;
        });

        scene->engine.graphicsEngine->submitHUDItems(drawItems);
    }

} // namespace engine::ecs
