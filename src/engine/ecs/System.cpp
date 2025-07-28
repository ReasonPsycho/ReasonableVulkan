//
// Created by redkc on 23/02/2024.
//

#include "SystemManager.h"
#include "System.h"
#include "tracy/Tracy.hpp"
using namespace engine::ecs;

System::System() {
}

SystemManager *System::getSystemManager() {
    return systemManager;
}


void System::Update() {
    ZoneTransientN(zoneName,(name).c_str(),true);
    UpdateImpl();
}

