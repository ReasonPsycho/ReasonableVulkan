#ifndef REASONABLEGL_ENTITY_TPP
#define REASONABLEGL_ENTITY_TPP

#include "../Scene.h"
#include <cassert>

namespace engine::ecs {

    template<typename T, typename... Args>
    T& Entity::AddComponent(Args&&... args) {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        if constexpr (sizeof...(Args) == 0) {
            m_Scene->AddComponent<T>(m_ID);
        } else {
            m_Scene->AddComponent<T>(m_ID, T(std::forward<Args>(args)...));
        }
        return m_Scene->GetComponent<T>(m_ID);
    }

    template<typename T>
    T& Entity::GetComponent() {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        return m_Scene->GetComponent<T>(m_ID);
    }

    template<typename T>
    const T& Entity::GetComponent() const {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        return m_Scene->GetComponent<T>(m_ID);
    }

    template<typename T>
    bool Entity::HasComponent() const {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        return m_Scene->HasComponent<T>(m_ID);
    }

    template<typename T>
    void Entity::RemoveComponent() {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        m_Scene->RemoveComponent<T>(m_ID);
    }

    template<typename T>
    void Entity::SetComponentActive(bool active) {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        m_Scene->SetComponentActive<T>(m_ID, active);
    }

    template<typename T>
    bool Entity::IsComponentActive() const {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        return m_Scene->IsComponentActive<T>(m_ID);
    }

    inline void Entity::Destroy() {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        m_Scene->DestroyEntity(m_ID);
    }

    inline bool Entity::IsActive() const {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        return m_Scene->IsEntityActive(m_ID);
    }

    inline void Entity::SetActive(bool active) {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        m_Scene->SetEntityActive(m_ID, active);
    }

    inline std::string Entity::GetName() const {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        return m_Scene->GetEntityName(m_ID);
    }

    inline void Entity::SetName(const std::string& name) {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        m_Scene->SetEntityName(m_ID, name);
    }

    inline bool Entity::HasParent() const {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        return m_Scene->HasParent(m_ID);
    }

    inline Entity Entity::GetParent() const {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        EntityID parentId = m_Scene->GetParent(m_ID);
        return Entity(parentId, m_Scene);
    }

    inline void Entity::SetParent(Entity parent) {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        m_Scene->SetParent(m_ID, parent.GetID());
    }

    inline void Entity::RemoveParent() {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        m_Scene->RemoveParent(m_ID);
    }

    inline std::vector<Entity> Entity::GetChildren() const {
        assert(m_Scene != nullptr && "Entity has no associated Scene!");
        auto childIds = m_Scene->GetChildren(m_ID);
        std::vector<Entity> children;
        children.reserve(childIds.size());
        for (auto childId : childIds) {
            children.emplace_back(childId, m_Scene);
        }
        return children;
    }

} // namespace engine::ecs

#endif // REASONABLEGL_ENTITY_TPP
