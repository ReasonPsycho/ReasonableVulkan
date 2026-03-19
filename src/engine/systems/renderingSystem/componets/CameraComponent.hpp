
#ifndef CAMERA_H
#define CAMERA_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <boost/uuid/nil_generator.hpp>
#include "ecs/Component.hpp"

namespace engine::ecs
{
    struct CameraComponent : public Component
    {
        // Camera parameters
        float fov;
        float aspectRatio;  // 16:9 by default
        float nearPlane;
        float farPlane;
        
        // Cached matrices
        glm::mat4 projection;
        glm::mat4 view;
        boost::uuids::uuid skyboxMaterialId;

        bool isDirty = true;
        bool active = false;
        CameraComponent() : Component(), fov(45.0f), aspectRatio(1.77f), nearPlane(0.1f), farPlane(1000.0f),projection(1.0f), view(1.0f), skyboxMaterialId(boost::uuids::nil_uuid()), active(false) {}

        void ShowImGui(Scene* scene,Component* component) const override;

        void SerializeComponentToJson(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
        void DeserializeComponentFromJson(const rapidjson::Value& obj) override;
    };


inline void updateProjectionMatrix(CameraComponent& camera)
    {
        camera.projection = glm::perspective(glm::radians(camera.fov),
                                          camera.aspectRatio,
                                          camera.nearPlane,
                                          camera.farPlane);
        camera.isDirty = false;
    }

    inline void updateViewMatrix(CameraComponent& camera, const glm::mat4& transformMatrix)
    {
        camera.view = glm::inverse(transformMatrix);
    }

    // Setters
    inline void setFov(CameraComponent& camera, float fov)
    {
        camera.fov = fov;
        camera.isDirty = true;
    }

    inline void setAspectRatio(CameraComponent& camera, float aspectRatio)
    {
        camera.aspectRatio = aspectRatio;
        camera.isDirty = true;
    }

    inline void setNearPlane(CameraComponent& camera, float nearPlane)
    {
        camera.nearPlane = nearPlane;
        camera.isDirty = true;
    }

    inline void setFarPlane(CameraComponent& camera, float farPlane)
    {
        camera.farPlane = farPlane;
        camera.isDirty = true;
    }

    // Getters
    inline float getFov(const CameraComponent& camera)
    {
        return camera.fov;
    }

    inline float getAspectRatio(const CameraComponent& camera)
    {
        return camera.aspectRatio;
    }

    inline float getNearPlane(const CameraComponent& camera)
    {
        return camera.nearPlane;
    }

    inline float getFarPlane(const CameraComponent& camera)
    {
        return camera.farPlane;
    }

    inline const glm::mat4& getProjectionMatrix(CameraComponent& camera)
    {
        if (camera.isDirty) {
            updateProjectionMatrix(camera);
        }
        return camera.projection;
    }

    inline const glm::mat4& getViewMatrix(const CameraComponent& camera)
    {
        return camera.view;
    }

    inline void setSkyboxTextureId(CameraComponent& camera, const boost::uuids::uuid& textureId)
    {
        camera.skyboxMaterialId = textureId;
    }

    inline const boost::uuids::uuid& getSkyboxTextureId(const CameraComponent& camera)
    {
        return camera.skyboxMaterialId;
    }

    inline bool isDirty(const CameraComponent& camera)
    {
        return camera.isDirty;
    }

}
#endif //CAMERA_H
