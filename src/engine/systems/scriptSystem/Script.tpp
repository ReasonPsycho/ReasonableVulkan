#include "ecs/Scene.h"

namespace engine::ecs {

    template<typename T, typename... Args>
    T& Script::AddComponent(Args&&... args) {
        return GetEntity().AddComponent<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    T& Script::GetComponent() {
        return GetEntity().GetComponent<T>();
    }

    template<typename T>
    const T& Script::GetComponent() const {
        return GetEntity().GetComponent<T>();
    }

    template<typename T>
    bool Script::HasComponent() const {
        return GetEntity().HasComponent<T>();
    }

    template<typename T>
    void Script::RemoveComponent() {
        GetEntity().RemoveComponent<T>();
    }

} // namespace engine::ecs
