
#ifndef CAMERA_H
#define CAMERA_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ecs/Component.hpp"

namespace engine::ecs
{
    struct Camera : public Component
    {
        // Camera parameters
        float fov;
        float aspectRatio;  // 16:9 by default
        float nearPlane;
        float farPlane;
        
        // Cached matrices
        glm::mat4 projection;
        glm::mat4 view;
        glm::vec4 lightpos;

        bool isDirty = true;
        Camera() : Component(), fov(45.0f), aspectRatio(1.77f), nearPlane(0.1f), farPlane(1000.0f),projection(1.0f), view(1.0f),lightpos(0.0f) {}
    };

    // Utility functions
    inline void updateProjectionMatrix(Camera& camera)
    {
        camera.projection = glm::perspective(glm::radians(camera.fov), 
                                          camera.aspectRatio, 
                                          camera.nearPlane, 
                                          camera.farPlane);
        camera.isDirty = false;
    }

    inline void updateViewMatrix(Camera& camera, const glm::mat4& transformMatrix)
    {
        camera.view = glm::inverse(transformMatrix);
    }

    // Setters
    inline void setFov(Camera& camera, float fov)
    {
        camera.fov = fov;
        camera.isDirty = true;
    }

    inline void setAspectRatio(Camera& camera, float aspectRatio)
    {
        camera.aspectRatio = aspectRatio;
        camera.isDirty = true;
    }

    inline void setNearPlane(Camera& camera, float nearPlane)
    {
        camera.nearPlane = nearPlane;
        camera.isDirty = true;
    }

    inline void setFarPlane(Camera& camera, float farPlane)
    {
        camera.farPlane = farPlane;
        camera.isDirty = true;
    }

    inline void setLightPosition(Camera& camera, const glm::vec4& lightPos)
    {
        camera.lightpos = lightPos;
    }

    // Getters
    inline float getFov(const Camera& camera)
    {
        return camera.fov;
    }

    inline float getAspectRatio(const Camera& camera)
    {
        return camera.aspectRatio;
    }

    inline float getNearPlane(const Camera& camera)
    {
        return camera.nearPlane;
    }

    inline float getFarPlane(const Camera& camera)
    {
        return camera.farPlane;
    }

    inline const glm::mat4& getProjectionMatrix(Camera& camera)
    {
        if (camera.isDirty) {
            updateProjectionMatrix(camera);
        }
        return camera.projection;
    }

    inline const glm::mat4& getViewMatrix(const Camera& camera)
    {
        return camera.view;
    }

    inline const glm::vec4& getLightPosition(const Camera& camera)
    {
        return camera.lightpos;
    }

    inline bool isDirty(const Camera& camera)
    {
        return camera.isDirty;
    }
}
#endif //CAMERA_H