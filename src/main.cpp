#define SDL_MAIN_HANDLED

#include <iostream>
#include <cstdlib>
#include <glm/ext/matrix_transform.hpp>
#include <SDL3/SDL.h>

#include "Asset.hpp"
#include "assetDatas/MaterialData.h"
#include "assetDatas/MeshData.h"
#include "assetDatas/ModelData.h"
#include "assetDatas/TextureData.h"
#include "platform/src/Platform.hpp"
#include "platform/include/PlatformInterface.hpp"
#include "assetManager/src/AssetManager.hpp"
#include "ecs/Scene.h"
#include "EngineInterface.hpp"
#include "engine/Engine.h"
#include "systems/renderingSystem/componets/CameraComponent.hpp"
#include "systems/renderingSystem/componets/LightComponent.hpp"
#include "systems/renderingSystem/componets/MeshComponent.hpp"
#include "systems/renderingSystem/componets/RendererComponent.hpp"
#include "vks/VulkanRenderer.h"
#include "Config.hpp"


namespace am
{
    struct MaterialData;
    struct TextureData;
}

int main(int argc, char *argv[]) {
    am::AssetManagerInterface& assetManager = am::AssetManager::getInstance();

    plt::PlatformInterface* platform = new plt::Platform();
    // 1. Initialize platform (SDL window, input, etc.)
    if (!platform->Init("My Game Engine", &assetManager)) {
        return EXIT_FAILURE;
    }

    assetManager.Initialize(platform);

    vks::VulkanRenderer *vulkanRenderer = new vks::VulkanRenderer(&assetManager);

    int width, height;
    platform->GetWindowSize(width, height);
    vulkanRenderer->initialize(platform, width, height);

    engine::EngineInterface* engine = new engine::Engine(platform, vulkanRenderer, &assetManager);
    engine->Initialize();

    assetManager.setEngine(engine);
    // 5. Main loop
    bool running = true;
    while (running) {
        platform->PollEvents(running); // sets `running` to false on quit

        float deltaTime = platform->GetDeltaTime();
        engine->Update(deltaTime);     // game logic
    }

    delete engine;
    delete vulkanRenderer;
    platform->Shutdown();
    delete platform;

    return EXIT_SUCCESS;
}
