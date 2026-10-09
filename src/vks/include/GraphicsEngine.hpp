
#ifndef GFX_HPP
#define GFX_HPP
#include <boost/mp11/integral.hpp>
#include <boost/uuid/uuid.hpp>
#include <glm/fwd.hpp>
#include <glm/detail/type_mat4x4.hpp>
#include <string>

#include "Handle.hpp"
#include "LightData.hpp"


namespace plt
{
    class PlatformInterface;
}

namespace gfx {
    class GraphicsEngine {
    protected:
        GraphicsEngine() = default;

    public:
        virtual ~GraphicsEngine() = default;

        virtual void initialize(plt::PlatformInterface* platform, uint32_t width, uint32_t height) = 0;
        virtual void resize(uint32_t width, uint32_t height) {}
        virtual glm::uvec2 getExtent() { return glm::uvec2(0, 0); }
        virtual void* getViewportTexturePointer() = 0;
        virtual void* getViewportTexturePointer(uint32_t cameraIndex) = 0;

        virtual void setCameraData(uint32_t cameraIndex, const glm::mat4& projection, const glm::mat4& view, const glm::vec3 cameraPos) = 0;
        virtual void setActiveCameraCount(uint32_t count) = 0;

        // Rendering commands using Handles
        virtual void drawModel(uint32_t cameraIndex, MeshHandle meshHandle, MaterialHandle materialHandle, ShaderProgramHandle shaderHandle, const glm::mat4& transform) = 0;
        virtual void drawSkybox(uint32_t cameraIndex, MaterialHandle materialHandle, ShaderProgramHandle shaderHandle) = 0;
        virtual void drawLight(PointLightData pointLightData, const glm::mat4& transform) = 0;
        virtual void drawLight(SpotLightData spotLightData, const glm::mat4& transform) = 0;
        virtual void drawLight(DirectionalLightData directionalLightData, const glm::mat4& transform) = 0;

        // Asset registration / handle acquisition
        virtual ModelHandle loadModel(boost::uuids::uuid uuid) = 0;
        virtual MeshHandle loadMesh(boost::uuids::uuid uuid) = 0;
        virtual ShaderProgramHandle loadShader(boost::uuids::uuid uuid) = 0;
        virtual TextureHandle loadTexture(boost::uuids::uuid uuid) = 0;
        virtual MaterialHandle loadMaterial(boost::uuids::uuid uuid) = 0;

        virtual ModelHandle getModelHandle(const std::string& lookUpName) = 0;
        virtual MeshHandle getMeshHandle(const std::string& lookUpName) = 0;
        virtual ShaderProgramHandle getShaderHandle(const std::string& lookUpName) = 0;
        virtual MaterialHandle getMaterialHandle(const std::string& lookUpName) = 0;

        virtual void* getThumbnailTexture(const boost::uuids::uuid& id, const std::string& thumbnailPath) { return nullptr; }
        virtual bool captureOffscreenImage(uint32_t cameraIndex, const std::string& outputPath, int targetWidth = 128, int targetHeight = 128) { return false; }
        virtual bool renderAndCaptureModelThumbnail(
            const boost::uuids::uuid& modelId,
            const std::string& outputPath,
            int targetWidth,
            int targetHeight,
            const glm::mat4& viewMatrix,
            const glm::mat4& projMatrix,
            const glm::vec3& camPos,
            const glm::vec3& lightDir,
            const glm::vec3& lightColor,
            float lightIntensity) { return false; }
        virtual bool renderAndCaptureMaterialThumbnail(
            const boost::uuids::uuid& materialId,
            const boost::uuids::uuid& sphereModelId,
            const std::string& outputPath,
            int targetWidth,
            int targetHeight,
            const glm::mat4& viewMatrix,
            const glm::mat4& projMatrix,
            const glm::vec3& camPos,
            const glm::vec3& lightDir,
            const glm::vec3& lightColor,
            float lightIntensity) { return false; }

        virtual void beginFrame() = 0;
        virtual void renderFrame() = 0;
        virtual void endFrame() = 0;
    };
} // namespace gfx

#endif //GFX_HPP