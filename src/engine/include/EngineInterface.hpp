//
// Created by redkc on 22/09/2026.
//

#ifndef REASONABLEVULKAN_ENGINEINTERFACE_HPP
#define REASONABLEVULKAN_ENGINEINTERFACE_HPP

#include <memory>
#include <string>
#include <vector>
#include <boost/uuid/uuid.hpp>

namespace plt {
    class PlatformInterface;
}

namespace gfx {
    class GraphicsEngine;
}

namespace am {
    class AssetManagerInterface;
}

namespace engine::ecs {
    class Scene;
    class EditorSystem;
}

namespace engine {
    class EngineInterface {
    public:
        virtual ~EngineInterface() = default;

        virtual void Initialize() = 0;

        // Scene management
        virtual std::shared_ptr<ecs::Scene> CreateScene(const std::string& name) = 0;
        virtual std::shared_ptr<ecs::Scene> GetScene(const std::string& name) = 0;
        virtual void RemoveScene(const std::string& name) = 0;
        virtual void SetActiveScene(const std::string& name, bool active = true) = 0;
        virtual void SetSceneActive(const std::string& name, bool active) = 0;
        virtual std::shared_ptr<ecs::Scene> GetActiveScene() = 0;
        virtual std::vector<std::shared_ptr<ecs::Scene>> GetActiveScenes() = 0;
        virtual const std::unordered_map<std::string, std::shared_ptr<ecs::Scene>>& GetScenes() const = 0;
        virtual std::unordered_map<std::string, std::shared_ptr<ecs::Scene>>& GetScenes() = 0;

        virtual std::shared_ptr<ecs::EditorSystem> GetEditorSystem() = 0;

        virtual void SaveScene() = 0;
        virtual void LoadScene(boost::uuids::uuid sceneId) = 0;
        virtual void CloseScene(const std::string& name) = 0;
        virtual void CloseScene(boost::uuids::uuid sceneId) = 0;

        virtual void* GetThumbnailTexture(const boost::uuids::uuid& assetId, const std::string& thumbnailPath) { return nullptr; }

        // Global update loop
        virtual void Update(float deltaTime) = 0;

    protected:
        EngineInterface() = default;
    };
}

#endif //REASONABLEVULKAN_ENGINEINTERFACE_HPP
