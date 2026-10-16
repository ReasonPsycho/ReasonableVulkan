#pragma once
#include <boost/test/unit_test.hpp>
#include "../Engine.h"
#include "ecs/Scene.h"
#include "ecs/System.h"
#include "ecs/Types.h"
#include "../assetManager/src/AssetManager.hpp"

namespace engine::test {

inline Engine CreateTestEngine() {
    return Engine(nullptr, nullptr, &am::AssetManager::getInstance());
}

} // namespace engine::test
