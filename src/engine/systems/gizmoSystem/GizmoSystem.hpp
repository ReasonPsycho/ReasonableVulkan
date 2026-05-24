//
// Created by redkc on 23/05/2026.
//

#ifndef REASONABLEVULKAN_GIZMOSYSTEM_HPP
#define REASONABLEVULKAN_GIZMOSYSTEM_HPP
#include <typeindex>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "ecs/System.h"

namespace engine::ecs
{
    struct Component;

    enum GizmoType
    {
        RAY,
        CUBE
    };

    struct GizmoRenderCommand
    {
        GizmoType type;
        glm::mat4 transform;
        glm::vec3 color;
        float lifetime;
        bool isNew;
    };

    class GizmoSystem :  public System<GizmoSystem>
    {
    public:
        explicit GizmoSystem(Scene* scene);

        void Initialize();

        void Update(float deltaTime) override;

        void DrawRay(glm::vec3 startPos, glm::vec3 endPos, glm::vec3 color);
        void DrawRay(glm::vec3 startPos, glm::vec3 endPos, glm::vec3 color, float duration);
        void DrawCube(glm::vec3 pos, glm::vec3 scale, glm::vec3 color);
        void DrawCube(glm::vec3 pos, glm::vec3 scale, glm::vec3 color, float duration);

        std::vector<GizmoRenderCommand> gizmoRenderCommandQueue;

        boost::uuids::uuid rayAssetUuid = boost::uuids::nil_uuid();
        boost::uuids::uuid cubeAssetUuid = boost::uuids::nil_uuid();

        boost::uuids::uuid rayShaderUuid = boost::uuids::nil_uuid();
        boost::uuids::uuid cubeShaderUuid = boost::uuids::nil_uuid();

        boost::uuids::uuid ModelUUIDByGizmoType(GizmoType type);
        boost::uuids::uuid ShaderUUIDByGizmoType(GizmoType type);

    protected:
        void OnComponentAdded(ComponentID componentID, std::type_index type) override {}
        void OnEntityRemoved(ComponentID componentID, std::type_index type) override {}
    };
} // engine

#endif //REASONABLEVULKAN_GIZMOSYSTEM_HPP
