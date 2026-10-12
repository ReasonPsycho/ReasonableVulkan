#include "ecs/Scene.h"

namespace engine::ecs {

    template<typename T>
    T& Script::GetComponent() {
        return scene->GetComponent<T>(entity);
    }

    template<typename T>
    bool Script::HasComponent() const {
        return scene->HasComponent<T>(entity);
    }

} // namespace engine::ecs
