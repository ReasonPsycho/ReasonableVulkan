
#ifndef CAMERA_H
#define CAMERA_H
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <variant>
#include "Handle.hpp"
#include "ecs/Component.hpp"

namespace engine::ecs
{
    struct SkyboxImageData
    {
        [[=Tooltip{"Skybox Material/Texture UUID"}, =LookupName{am::AssetType::Material}]]
        boost::uuids::uuid skyboxMaterialId{boost::uuids::nil_uuid()};

        [[=NonSerialized{}]]
        gfx::MaterialHandle runtimeSkyboxMaterialHandle = gfx::MaterialHandle::invalid();
    };

    struct SkyboxColorData
    {
        [[=Color{}, =Tooltip{"Background clear color"}]]
        glm::vec4 color{0.0f, 0.0f, 0.0f, 1.0f};
    };

    struct CameraComponent : public Component
    {
        enum class SkyboxType { Image, Color };

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

        [[=Tooltip{"Skybox type"}]]
        SkyboxType skyboxType{SkyboxType::Image};

        [[=Tooltip{"Type-specific skybox settings"}]]
        std::variant<SkyboxImageData, SkyboxColorData> skyboxData{SkyboxImageData{}};

        [[=NonSerialized{}, =HidenInInspector{}]]
        bool isDirty{true};

        [[=Tooltip{"Whether camera is active"}]]
        bool active{false};

        [[=Tooltip{"Show gizmo in editor"}]]
        bool showGizmo{true};

        SkyboxType getSkyboxType() const { return skyboxType; }
        void setSkyboxType(SkyboxType t) {
            switch (t) {
                case SkyboxType::Image:
                    if (!std::holds_alternative<SkyboxImageData>(skyboxData)) skyboxData = SkyboxImageData{};
                    break;
                case SkyboxType::Color:
                    if (!std::holds_alternative<SkyboxColorData>(skyboxData)) skyboxData = SkyboxColorData{};
                    break;
            }
            skyboxType = t;
        }

        void PostDeserialize() {
            setSkyboxType(skyboxType);
        }

        void PostModify() {
            setSkyboxType(skyboxType);
        }

        void CustomDeserialize(const rapidjson::Value& inObj) {
            if (!inObj.IsObject()) return;
            if (inObj.HasMember("fov")) DeserializeValueFromJson(fov, inObj["fov"]);
            if (inObj.HasMember("aspectRatio")) DeserializeValueFromJson(aspectRatio, inObj["aspectRatio"]);
            if (inObj.HasMember("nearPlane")) DeserializeValueFromJson(nearPlane, inObj["nearPlane"]);
            if (inObj.HasMember("farPlane")) DeserializeValueFromJson(farPlane, inObj["farPlane"]);
            if (inObj.HasMember("active")) DeserializeValueFromJson(active, inObj["active"]);
            if (inObj.HasMember("showGizmo")) DeserializeValueFromJson(showGizmo, inObj["showGizmo"]);
            if (inObj.HasMember("skyboxType")) {
                DeserializeValueFromJson(skyboxType, inObj["skyboxType"]);
                setSkyboxType(skyboxType);
            }
            if (inObj.HasMember("skyboxData")) {
                DeserializeValueFromJson(skyboxData, inObj["skyboxData"]);
            } else if (inObj.HasMember("skyboxMaterialId")) {
                skyboxType = SkyboxType::Image;
                SkyboxImageData imgData{};
                DeserializeValueFromJson(imgData.skyboxMaterialId, inObj["skyboxMaterialId"]);
                skyboxData = imgData;
            }
            isDirty = true;
        }

        CameraComponent() = default;
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
        camera.setSkyboxType(CameraComponent::SkyboxType::Image);
        if (auto* imgData = std::get_if<SkyboxImageData>(&camera.skyboxData)) {
            imgData->skyboxMaterialId = textureId;
            imgData->runtimeSkyboxMaterialHandle = gfx::MaterialHandle::invalid();
        }
        camera.isDirty = true;
    }

    inline boost::uuids::uuid getSkyboxTextureId(const CameraComponent& camera)
    {
        if (auto* imgData = std::get_if<SkyboxImageData>(&camera.skyboxData)) {
            return imgData->skyboxMaterialId;
        }
        return boost::uuids::nil_uuid();
    }

    inline void setSkyboxColor(CameraComponent& camera, const glm::vec4& color)
    {
        camera.setSkyboxType(CameraComponent::SkyboxType::Color);
        if (auto* colData = std::get_if<SkyboxColorData>(&camera.skyboxData)) {
            colData->color = color;
        }
        camera.isDirty = true;
    }

    inline glm::vec4 getSkyboxColor(const CameraComponent& camera)
    {
        if (auto* colData = std::get_if<SkyboxColorData>(&camera.skyboxData)) {
            return colData->color;
        }
        return glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    inline bool isDirty(const CameraComponent& camera)
    {
        return camera.isDirty;
    }

    inline bool getShowGizmo(const CameraComponent& camera)
    {
        return camera.showGizmo;
    }

    inline void setShowGizmo(CameraComponent& camera, bool showGizmo)
    {
        camera.showGizmo = showGizmo;
    }

}
#endif //CAMERA_H
