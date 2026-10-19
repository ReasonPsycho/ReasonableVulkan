#include "HUDSystem.h"
#include "ecs/Scene.h"
#include "Engine.h"
#include "GraphicsEngine.hpp"
#include "PlatformInterface.hpp"
#include "systems/editorSystem/EditorSystem.hpp"
#include "systems/renderingSystem/componets/ThumbnailComponent.hpp"
#include <algorithm>
#include <vector>

namespace engine::ecs {

    std::pair<glm::vec2, glm::vec2> HUDSystem::ComputeScreenRect(Entity entity, float screenWidth, float screenHeight) const {
        if (!scene || !scene->HasComponent<RectTransformComponent>(entity)) {
            return {glm::vec2(0.0f), glm::vec2(screenWidth, screenHeight)};
        }

        const auto& rect = scene->GetComponent<RectTransformComponent>(entity);
        glm::vec2 parentPos{0.0f, 0.0f};
        glm::vec2 parentSize{screenWidth, screenHeight};

        if (scene->HasParent(entity)) {
            Entity parent = scene->GetParent(entity);
            if (scene->HasComponent<RectTransformComponent>(parent)) {
                auto parentRect = ComputeScreenRect(parent, screenWidth, screenHeight);
                parentPos = parentRect.first;
                parentSize = parentRect.second;
            }
        }

        glm::vec2 anchorMinPx = parentPos + parentSize * rect.anchorMin;
        glm::vec2 anchorMaxPx = parentPos + parentSize * rect.anchorMax;
        glm::vec2 anchorSpan = anchorMaxPx - anchorMinPx;

        bool isPointAnchor = (rect.anchorMin == rect.anchorMax);
        glm::vec2 finalSize = isPointAnchor ? rect.size : (anchorSpan + rect.size);
        glm::vec2 finalPos = anchorMinPx + rect.position - rect.pivot * (isPointAnchor ? rect.size : glm::vec2(0.0f));

        return {finalPos, finalSize};
    }

    void HUDSystem::Update(float deltaTime) {
        if (scene->engine.minimized || scene->engine.graphicsEngine == nullptr) {
            return;
        }

        int width = 0, height = 0;
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

        auto editorSystem = scene->engine.GetEditorSystem();
        bool inEditMode = editorSystem ? editorSystem->inEditMode : false;
        auto activeScenes = scene->engine.GetActiveScenes();
        uint32_t sceneIndex = 0;
        for (size_t s = 0; s < activeScenes.size(); ++s) {
            if (activeScenes[s].get() == scene) {
                sceneIndex = static_cast<uint32_t>(s);
                break;
            }
        }

        std::vector<uint32_t> targetCameraIndices;
        auto thumbCompArray = scene->GetComponentArray<ThumbnailComponent>();
        bool isThumbnailScene = (scene->GetSceneType() == ecs::SceneType::Thumbnail) || (thumbCompArray && thumbCompArray->GetArraySize() > 0);

        if (inEditMode && editorSystem && !isThumbnailScene) {
            targetCameraIndices.push_back(sceneIndex);
            auto topEditableScene = scene->engine.GetTopEditableScene();
            if (topEditableScene && topEditableScene.get() == scene) {
                targetCameraIndices.push_back(static_cast<uint32_t>(activeScenes.size()));
            }
        } else {
            targetCameraIndices.push_back(sceneIndex);
        }

        struct UIEntityData {
            Entity entity;
            int zIndex;
            glm::vec2 pos;
            glm::vec2 size;
        };

        std::vector<UIEntityData> activeUIEntities;

        for (auto [entity, rect, image] : scene->GetActiveView<RectTransformComponent, UIImageComponent>()) {
            if (!image.visible) continue;

            if (image.isDirty) {
                image.runtimeTextureHandle = gfx::TextureHandle::invalid();
                image.isDirty = false;
            }

            if (image.textureUuid != boost::uuids::nil_uuid() && !image.runtimeTextureHandle.isValid()) {
                image.runtimeTextureHandle = scene->engine.graphicsEngine->loadTexture(image.textureUuid);
            }

            auto [pos, size] = ComputeScreenRect(entity, static_cast<float>(width), static_cast<float>(height));
            activeUIEntities.push_back({entity, rect.zIndex, pos, size});
        }

        std::stable_sort(activeUIEntities.begin(), activeUIEntities.end(), [](const UIEntityData& a, const UIEntityData& b) {
            return a.zIndex < b.zIndex;
        });

        for (const auto& item : activeUIEntities) {
            auto& image = scene->GetComponent<UIImageComponent>(item.entity);

            gfx::UIQuadData quadData{};
            quadData.screenPosition = item.pos;
            quadData.size = item.size;
            quadData.color = image.color;
            quadData.uvRect = image.uvRect;
            quadData.texture = image.runtimeTextureHandle;
            quadData.layer = item.zIndex;

            for (uint32_t camIdx : targetCameraIndices) {
                scene->engine.graphicsEngine->drawUIQuad(camIdx, quadData);
            }
        }
    }
}
