#include "Engine.h"

#include "PlatformInterface.hpp"
#include "ecs/Scene.h"
#include "systems/collisionSystem/CollisionSystem.hpp"
#include "systems/editorSystem/EditorSystem.hpp"
#include "systems/lightSystem/LightSystem.hpp"
#include "systems/lightSystem/components/DirectionalLight.hpp"
#include "systems/lightSystem/components/PointLight.hpp"
#include "systems/lightSystem/components/SpotLight.hpp"
#include "systems/renderingSystem/RenderSystem.h"
#include "systems/transformSystem/TransformSystem.h"

namespace engine {
    Engine::Engine(plt::PlatformInterface* platformInterface, gfx::GraphicsEngine* graphicsEngine,
                   am::AssetManagerInterface* assetManagerInterface) : assetManagerInterface(assetManagerInterface), graphicsEngine(graphicsEngine),
                                                                       platform(platformInterface)
    {
        RegisterSystemType<RenderSystem>();
        RegisterComponentType<Model>(); //For some reason I have to register them in reverse
        RegisterComponentType<Camera>();

#ifdef EDITOR_ENABLED
        RegisterSystemType<EditorSystem>();
#endif

        RegisterSystemType<TransformSystem>();
        RegisterComponentType<Transform>();

        RegisterSystemType<CollisionSystem>();

        RegisterSystemType<LightSystem>();
        RegisterComponentType<PointLight>();
        RegisterComponentType<DirectionalLight>();
        RegisterComponentType<SpotLight>();

    }



    void Engine::Initialize()
    {
        platform->SubscribeToEvent(plt::EventType::WindowMinimize,
      [this](const void* /*data*/) {
          minimized = true;
      });

        platform->SubscribeToEvent(plt::EventType::WindowRestored,
            [this](const void* /*data*/) {
                minimized = false;
            });
    }

    std::shared_ptr<Scene> Engine::CreateScene(const std::string& name) {
        if (scenes.find(name) != scenes.end()) {
            return scenes[name]; // Scene already exists, return it
        }

        auto scene = std::make_shared<Scene>(*this);  // Changed from (this) to (*this)
        scenes[name] = scene;

        // Optionally set it active if it's the first one
        if (!activeScene) {
            activeScene = scene;
        }

        return scene;
    }

    std::shared_ptr<Scene> Engine::GetScene(const std::string& name) {
        auto it = scenes.find(name);
        if (it != scenes.end()) {
            return it->second;
        }
        return nullptr;
    }

    void Engine::RemoveScene(const std::string& name) {
        auto it = scenes.find(name);
        if (it != scenes.end()) {
            if (activeScene == it->second) {
                activeScene = nullptr;
            }
            scenes.erase(it);
        }
    }

    void Engine::SetActiveScene(const std::string& name) {
        auto it = scenes.find(name);
        if (it != scenes.end()) {
            activeScene = it->second;
        }
    }

    std::shared_ptr<Scene> Engine::GetActiveScene() {
        return activeScene;
    }

    void Engine::Update(float deltaTime) {
        if (activeScene) {
            activeScene->Update(deltaTime);
        }
    }

    std::shared_ptr<IComponentArray> Engine::CreateComponentArray(const std::type_index& type) const
    {
        auto it = componentFactories.find(type);
        if (it != componentFactories.end()) {
            return it->second();
        }
        throw std::runtime_error("No factory registered for component type: " + std::string(type.name()));
    }

    std::shared_ptr<SystemBase> Engine::CreateSystem(const std::type_index& type, Scene* scene) const
    {
        auto it = systemFactories.find(type);
        if (it != systemFactories.end()) {
            return it->second(scene);
        }
        throw std::runtime_error("No factory registered for system type: " + std::string(type.name()));
    }

    void Engine::SaveScene(std::string filename)
    {
        try
        {
            rapidjson::Document document;
            document.SetObject();
            auto& allocator = document.GetAllocator();

            // Add encoding information
            rapidjson::Value encodingInfo(rapidjson::kObjectType);
            encodingInfo.AddMember("encoding", "UTF-8", allocator);
            encodingInfo.AddMember("version", "1.0", allocator);
            document.AddMember("_meta", encodingInfo, allocator);

            activeScene->SerializeToJson(document);

            // Convert to string with pretty printing
            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            writer.SetIndent(' ', 2);
            document.Accept(writer);

            // Save to file
            std::ofstream ofs(filename, std::ios::out | std::ios::binary);
            if (!ofs.is_open()) {
                spdlog::error("Failed to open file for writing: {}", filename);
                return;
            }

            // Write UTF-8 BOM
            const char bom[3] = { static_cast<char>(0xEF), static_cast<char>(0xBB), static_cast<char>(0xBF) };
            ofs.write(bom, 3);

            // Write the JSON content
            ofs.write(buffer.GetString(), buffer.GetSize());
            ofs.close();
        } catch (const std::exception& e) {
            spdlog::error("Error saving scene to file: {}", e.what());
        }
    }

    void Engine::LoadScene(std::string filename)
    {
        try {
            std::ifstream ifs(filename, std::ios::binary);
            if (!ifs.is_open()) {
                spdlog::error("Failed to open file for reading: {}", filename);
                return;
            }

            // Skip UTF-8 BOM if present
            char bom[3];
            ifs.read(bom, 3);
            if (!(bom[0] == static_cast<char>(0xEF) &&
                  bom[1] == static_cast<char>(0xBB) &&
                  bom[2] == static_cast<char>(0xBF))) {
                ifs.seekg(0);
                  }

            std::string json_content((std::istreambuf_iterator<char>(ifs)),
                                   std::istreambuf_iterator<char>());

            rapidjson::Document document;
            document.Parse(json_content.c_str());

           activeScene->DeserializeFromJson(document);

            return ;
        } catch (const std::exception& e) {
            spdlog::error("Error loading scene from file: {}", e.what());
            return;
        }

    }
} // namespace engine