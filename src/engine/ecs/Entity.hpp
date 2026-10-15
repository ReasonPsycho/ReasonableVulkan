#ifndef REASONABLEGL_ENTITY_HPP
#define REASONABLEGL_ENTITY_HPP

#include "EngineTypes.hpp"
#include <string>
#include <vector>
#include <concepts>

namespace engine::ecs {

    class Scene;

    class Entity {
    public:
        constexpr Entity() = default;
        constexpr Entity(EntityID id, Scene* scene = nullptr)
            : m_ID(id), m_Scene(scene) {}

        // Component accessors
        template<typename T, typename... Args>
        T& AddComponent(Args&&... args);

        template<typename T>
        T& GetComponent();

        template<typename T>
        const T& GetComponent() const;

        template<typename T>
        bool HasComponent() const;

        template<typename T>
        void RemoveComponent();

        template<typename T>
        void SetComponentActive(bool active);

        template<typename T>
        bool IsComponentActive() const;

        // Entity operations
        void Destroy();
        bool IsActive() const;
        void SetActive(bool active);

        std::string GetName() const;
        void SetName(const std::string& name);

        // Hierarchy operations
        bool HasParent() const;
        Entity GetParent() const;
        void SetParent(Entity parent);
        void RemoveParent();
        std::vector<Entity> GetChildren() const;

        // Identification & Scene access
        EntityID GetID() const { return m_ID; }
        Scene* GetScene() const { return m_Scene; }
        bool IsValid() const { return m_Scene != nullptr && m_ID != MAX_ENTITIES; }

        // Ergonomics: validity checks & implicit conversions
        explicit operator bool() const {
            return IsValid();
        }

        operator EntityID() const { return m_ID; }

        Entity& operator=(EntityID id) {
            m_ID = id;
            return *this;
        }

        bool operator==(const Entity& other) const {
            if (m_Scene != nullptr && other.m_Scene != nullptr && m_Scene != other.m_Scene) {
                return false;
            }
            return m_ID == other.m_ID;
        }

        bool operator!=(const Entity& other) const {
            return !(*this == other);
        }

        bool operator<(const Entity& other) const {
            return m_ID < other.m_ID;
        }

        bool operator<=(const Entity& other) const {
            return m_ID <= other.m_ID;
        }

        bool operator>(const Entity& other) const {
            return m_ID > other.m_ID;
        }

        bool operator>=(const Entity& other) const {
            return m_ID >= other.m_ID;
        }

        template<std::integral U>
        bool operator==(U other) const {
            return m_ID == static_cast<EntityID>(other);
        }

        template<std::integral U>
        bool operator!=(U other) const {
            return m_ID != static_cast<EntityID>(other);
        }

        template<std::integral U>
        bool operator<(U other) const {
            return m_ID < static_cast<EntityID>(other);
        }

        template<std::integral U>
        bool operator<=(U other) const {
            return m_ID <= static_cast<EntityID>(other);
        }

        template<std::integral U>
        bool operator>(U other) const {
            return m_ID > static_cast<EntityID>(other);
        }

        template<std::integral U>
        bool operator>=(U other) const {
            return m_ID >= static_cast<EntityID>(other);
        }

        template<std::integral U>
        friend bool operator==(U lhs, const Entity& rhs) {
            return static_cast<EntityID>(lhs) == rhs.m_ID;
        }

        template<std::integral U>
        friend bool operator!=(U lhs, const Entity& rhs) {
            return static_cast<EntityID>(lhs) != rhs.m_ID;
        }

        template<std::integral U>
        friend bool operator<(U lhs, const Entity& rhs) {
            return static_cast<EntityID>(lhs) < rhs.m_ID;
        }

        template<std::integral U>
        friend bool operator<=(U lhs, const Entity& rhs) {
            return static_cast<EntityID>(lhs) <= rhs.m_ID;
        }

        template<std::integral U>
        friend bool operator>(U lhs, const Entity& rhs) {
            return static_cast<EntityID>(lhs) > rhs.m_ID;
        }

        template<std::integral U>
        friend bool operator>=(U lhs, const Entity& rhs) {
            return static_cast<EntityID>(lhs) >= rhs.m_ID;
        }

    private:
        EntityID m_ID{MAX_ENTITIES};
        Scene* m_Scene{nullptr};
    };

} // namespace engine::ecs

namespace std {
    template<>
    struct hash<engine::ecs::Entity> {
        std::size_t operator()(const engine::ecs::Entity& entity) const noexcept {
            return std::hash<engine::ecs::EntityID>()(entity.GetID());
        }
    };
}

#endif // REASONABLEGL_ENTITY_HPP
