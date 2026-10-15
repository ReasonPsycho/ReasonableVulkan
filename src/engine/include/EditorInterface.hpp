//
// Created by redkc on 12/10/2026.
//

#ifndef REASONABLEVULKAN_EDITORINTERFACE_HPP
#define REASONABLEVULKAN_EDITORINTERFACE_HPP

#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <boost/uuid/uuid.hpp>
#include <glm/glm.hpp>
#include "EngineTypes.hpp"

namespace engine::ecs {
    class Scene;
}

namespace engine::editor {
    class EditorInterface {
    public:
        virtual ~EditorInterface() = default;

        // Lifecycle & updates
        virtual void Update(float deltaTime) = 0;
        virtual void ProcessThumbnailCaptures() = 0;

        // Thumbnails
        virtual void* GetThumbnailTexture(const boost::uuids::uuid& assetId, const std::string& thumbnailPath) = 0;
        virtual bool QueueThumbnailGeneration(const boost::uuids::uuid& assetId, const std::string& outputPath) = 0;
        virtual void InvalidateThumbnail(const boost::uuids::uuid& assetId) {}

        // Asset preview
        virtual std::shared_ptr<ecs::Scene> OpenAssetPreviewScene(const boost::uuids::uuid& assetId) = 0;

        // Prefab creation
        virtual bool SaveEntityAsPrefab(ecs::EntityID entity, const std::filesystem::path& destinationDirectoryOrPath, ecs::Scene* sourceScene = nullptr) = 0;

        // Entity naming
        virtual void SetEntityName(ecs::EntityID entity, const std::string& name, ecs::Scene* targetScene = nullptr) = 0;
        virtual std::string GetEntityName(ecs::EntityID entity, ecs::Scene* targetScene = nullptr) const = 0;
        virtual std::string GetEntityRawName(ecs::EntityID entity, ecs::Scene* targetScene = nullptr) const = 0;

        // Selection
        virtual ecs::EntityID GetSelectedEntity() const = 0;
        virtual void SetSelectedEntity(ecs::EntityID entity) = 0;
        virtual const std::vector<ecs::EntityID>& GetSelectedEntities() const = 0;
        virtual void SetSelectedEntities(const std::vector<ecs::EntityID>& entities) = 0;
        virtual void AddSelectedEntity(ecs::EntityID entity) = 0;
        virtual void RemoveSelectedEntity(ecs::EntityID entity) = 0;
        virtual void ClearSelectedEntities() = 0;
        virtual bool IsEntitySelected(ecs::EntityID entity) const = 0;

        // Copy / Paste / Duplicate
        virtual void CopySelectedEntities(ecs::Scene* sourceScene = nullptr) {}
        virtual void PasteEntities(ecs::Scene* targetScene = nullptr) {}
        virtual void PasteEntities(ecs::Scene* targetScene, ecs::EntityID parent) {}
        virtual void DuplicateSelectedEntities(ecs::Scene* targetScene = nullptr) {}
        virtual bool HasEntitiesInClipboard() const { return false; }

        // Target / active scene in editor
        virtual ecs::Scene* GetTargetScene() const = 0;
        virtual std::shared_ptr<ecs::Scene> GetSelectedScene() const = 0;
        virtual void SetTargetScene(std::shared_ptr<ecs::Scene> scene) = 0;

        // Camera control
        virtual void FocusCameraOnBounds(const glm::vec3& center, float distance, ecs::Scene* scene = nullptr) = 0;

        // History
        virtual void Undo() = 0;
        virtual void Redo() = 0;
        virtual bool CanUndo() const = 0;
        virtual bool CanRedo() const = 0;

        // Play/Pause mode
        virtual bool IsPlaying() const { return false; }
        virtual bool IsPaused() const { return false; }
        virtual void StartPlayMode() {}
        virtual void StopPlayMode() {}
        virtual void SetPaused(bool /*paused*/) {}
        virtual void TogglePlay() {}
        virtual void TogglePause() {}

    protected:
        EditorInterface() = default;
    };
}

namespace editor {
    using EditorInterface = engine::editor::EditorInterface;
}

namespace engine {
    using EditorInterface = engine::editor::EditorInterface;
}

#endif //REASONABLEVULKAN_EDITORINTERFACE_HPP
