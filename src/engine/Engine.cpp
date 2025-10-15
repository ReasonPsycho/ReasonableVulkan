#include "Engine.h"

#include "ecs/Scene.h"
#include "systems/editorSystem/EditorSystem.hpp"
#include "systems/renderingSystem/RenderSystem.h"
#include "systems/transformSystem/TransformSystem.h"

namespace engine {
    Engine::Engine(PlatformInterface* platformInterface, gfx::GraphicsEngine* graphicsEngine,
        am::AssetManagerInterface* assetManagerInterface): graphicsEngine(graphicsEngine), assetManagerInterface(assetManagerInterface), platform(platformInterface)
    {
            RegisterSystemType<RenderSystem>();

#ifdef EDITOR_ENABLED
            RegisterSystemType<EditorSystem>();

#endif
            RegisterComponentType<Model>(); //For some reason I have to register them in reverse
            RegisterComponentType<Camera>();

            RegisterComponentType<Transform>();
            RegisterSystemType<TransformSystem>();
    }

    void Engine::Initialize()
    {
        platform->SubscribeToEvent(PlatformInterface::EventType::WindowMinimize,
      [this](const void* /*data*/) {
          minimized = true;
      });

        platform->SubscribeToEvent(PlatformInterface::EventType::WindowRestored,
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