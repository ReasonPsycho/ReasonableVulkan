
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
        [[=Range{1.0f, 179.0f, 0.5f}, =Tooltip{"Field of view in degrees"}]]
        float fov{45.0f};

        [[=Range{0.01f, 10.0f, 0.01f}, =Tooltip{"Camera aspect ratio"}]]
        float aspectRatio{1.77f};  // 16:9 by default

        [[=Range{0.001f, 100.0f, 0.01f}, =Tooltip{"Near clipping plane"}]]
        float nearPlane{0.1f};

        [[=Range{1.0f, 10000.0f, 1.0f}, =Tooltip{"Far clipping plane"}]]
        float farPlane{1000.0f};
        
        // Cached matrices
        [[=NonSerialized{}, =ReadOnly{}, =Tooltip{"Projection matrix"}]]
        glm::mat4 projection{1.0f};

        [[=NonSerialized{}, =ReadOnly{}, =Tooltip{"View matrix"}]]
        glm::mat4 view{1.0f};

        [[=Tooltip{"Skybox Material/Texture UUID"}]]
        boost::uuids::uuid skyboxMaterialId{boost::uuids::nil_uuid()};

        [[=NonSerialized{}, =ReadOnly{}]]
        bool isDirty{true};

        [[=Tooltip{"Whether camera is active"}]]
        bool active{false};

        CameraComponent() = default;

        void PostDeserialize()
        {
            isDirty = true;
        }
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
