
#ifndef REASONABLEVULKAN_COLLISIONSYSTEM_HPP
#define REASONABLEVULKAN_COLLISIONSYSTEM_HPP

#include <typeindex>

#include "ecs/System.h"
#include <glm/glm.hpp>



namespace engine::ecs {
    struct Transform;
    struct Camera;

    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;
    };

    struct RayHit {
        Entity entity;
        float distance;
        glm::vec3 hitPoint;
    };

    class CollisionSystem : public System<CollisionSystem> {
    public:
        CollisionSystem(Scene* scene) : System(scene) {}
        void Update(float deltaTime) override {};
        // Convert screen coordinates to world ray
        Ray ScreenToWorldRay(const Camera& camera,
                             float screenX, float screenY, float windowWidth, float windowHeight);

        // Ray-AABB intersection test
        bool RayIntersectsAABB(const Ray& ray, const glm::vec3& boxMin, const glm::vec3& boxMax,
                              const glm::mat4& worldMatrix, float& outDistance);

        // Find closest entity hit by ray
        std::optional<RayHit> RayCastClosest(const Ray& ray);

    protected:
        void OnComponentAdded(ComponentID componentID, std::type_index type) override {}
        void OnEntityRemoved(ComponentID componentID, std::type_index type) override {}
    };

} // namespace engine::ecs

#endif //REASONABLEVULKAN_COLLISIONSYSTEM_HPP