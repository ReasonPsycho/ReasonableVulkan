#include "Engine.h"

template <typename T>
void Engine::RegisterComponentType()
{
    auto type = std::type_index(typeid(T));
    componentTypes.insert(type);
    componentFactories[type] = []() -> std::shared_ptr<IComponentArray> {
        if constexpr (std::is_same_v<T, Transform>) {
            return std::make_shared<IntegralComponentArray<T>>();
        } else {
            return std::make_shared<ComponentArray<T>>();
        }
    };
}


template <typename T>
void Engine::RegisterSystemType()
{
    auto type = std::type_index(typeid(T));
    systemTypes.insert(type);
    systemFactories[type] = [](Scene* scene) -> std::shared_ptr<SystemBase> {
        return std::make_shared<T>(scene);
    };
}