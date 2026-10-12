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

namespace engine::ecs {
    class Scene;
    using Entity = uint32_t;
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

        // Asset preview
        virtual std::shared_ptr<ecs::Scene> OpenAssetPreviewScene(const boost::uuids::uuid& assetId) = 0;

        // Prefab creation
        virtual bool SaveEntityAsPrefab(uint32_t entity, const std::filesystem::path& destinationDirectoryOrPath, ecs::Scene* sourceScene = nullptr) = 0;

        // Entity naming
        virtual void SetEntityName(ecs::Entity entity, const std::string& name, ecs::Scene* targetScene = nullptr) = 0;
        virtual std::string GetEntityName(ecs::Entity entity, ecs::Scene* targetScene = nullptr) const = 0;
        virtual std::string GetEntityRawName(ecs::Entity entity, ecs::Scene* targetScene = nullptr) const = 0;

        // Selection
        virtual ecs::Entity GetSelectedEntity() const = 0;
        virtual void SetSelectedEntity(ecs::Entity entity) = 0;
        virtual const std::vector<ecs::Entity>& GetSelectedEntities() const = 0;
        virtual void SetSelectedEntities(const std::vector<ecs::Entity>& entities) = 0;
        virtual void AddSelectedEntity(ecs::Entity entity) = 0;
        virtual void RemoveSelectedEntity(ecs::Entity entity) = 0;
        virtual void ClearSelectedEntities() = 0;
        virtual bool IsEntitySelected(ecs::Entity entity) const = 0;

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
