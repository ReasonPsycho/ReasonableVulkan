#include "CollisionSystem.hpp"

#include "ecs/Scene.h"
#include "systems/renderingSystem/componets/Camera.hpp"
#include "systems/renderingSystem/componets/Model.hpp"
#include "systems/transformSystem/componets/Transform.hpp"

namespace engine::ecs {

Ray CollisionSystem::ScreenToWorldRay(const Camera& camera,
                                      float screenX, float screenY, float windowWidth, float windowHeight) {
    // Convert screen coordinates to normalized device coordinates (-1 to 1)
    float x = (2.0f * screenX) / windowWidth - 1.0f;
    float y = (2.0f * screenY) / windowHeight - 1.0f;

    // Get view and projection matrices
    glm::mat4 projection = camera.projection;
    glm::mat4 view = camera.view;
    
    // Unproject near and far points
    glm::vec4 nearPoint = glm::inverse(projection * view) * glm::vec4(x, y, -1.0f, 1.0f);
    glm::vec4 farPoint = glm::inverse(projection * view) * glm::vec4(x, y, 1.0f, 1.0f);
    
    // Convert to 3D points
    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;
    
    Ray ray;
    ray.origin = glm::vec3(nearPoint);
    ray.direction = glm::normalize(glm::vec3(farPoint - nearPoint));
    
    return ray;
}

bool CollisionSystem::RayIntersectsAABB(const Ray& ray, const glm::vec3& boxMin, const glm::vec3& boxMax,
                                      const glm::mat4& worldMatrix, float& outDistance) {
    glm::vec3 modifiedDirection = ray.direction;
    const float epsilon = 1e-6f;  // Small value to avoid division by zero

    if (modifiedDirection.x == 0.0f) modifiedDirection.x = epsilon;
    if (modifiedDirection.y == 0.0f) modifiedDirection.y = epsilon;
    if (modifiedDirection.z == 0.0f) modifiedDirection.z = epsilon;

    glm::vec3 worldMin = glm::vec3(worldMatrix * glm::vec4(boxMin, 1.0f));
    glm::vec3 worldMax = glm::vec3(worldMatrix * glm::vec4(boxMax, 1.0f));
    
    glm::vec3 dirfrac = 1.0f / ray.direction;
    
    float t1 = (worldMin.x - ray.origin.x) * dirfrac.x;
    float t2 = (worldMax.x - ray.origin.x) * dirfrac.x;
    float t3 = (worldMin.y - ray.origin.y) * dirfrac.y;
    float t4 = (worldMax.y - ray.origin.y) * dirfrac.y;
    float t5 = (worldMin.z - ray.origin.z) * dirfrac.z;
    float t6 = (worldMax.z - ray.origin.z) * dirfrac.z;
    
    float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
    float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));
    
    if (tmax >= 0 && tmin <= tmax) {
        outDistance = tmin;
        return true;
    }
    
    return false;
}

std::optional<RayHit> CollisionSystem::RayCastClosest(const Ray& ray) {
    RayHit closestHit{};
    closestHit.distance = std::numeric_limits<float>::max();
    bool hasHit = false;

    auto modelArray = scene->GetComponentArray<Model>().get();
    auto& models = modelArray->GetComponents();
    auto& transforms = scene->GetIntegralComponentArray<Transform>().get()->GetComponents();

    // Only iterate up to the actual size of used components
    for (ComponentID i = 0; i < modelArray->GetArraySize(); i++)
    {
            Entity entity = modelArray->ComponentIndexToEntity(i);
            if (models[i].modelUuid != boost::uuids::nil_uuid())
            {
                float distance;
                if (RayIntersectsAABB(ray, models[i].boundingBoxMin, models[i].boundingBoxMax,
                                      transforms[entity].globalMatrix, distance))
                {
                    if (distance < closestHit.distance) {
                        closestHit.entity = entity;
                        closestHit.distance = distance;
                        closestHit.hitPoint = ray.origin + ray.direction * distance;
                        hasHit = true;
                    }
                }
            }
    }


    return hasHit ? std::optional<RayHit>(closestHit) : std::nullopt;
}

} // namespace engine::ecs