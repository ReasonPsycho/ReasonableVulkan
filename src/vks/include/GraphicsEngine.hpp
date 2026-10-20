
#ifndef GFX_HPP
#define GFX_HPP
#include <boost/mp11/integral.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <glm/fwd.hpp>
#include <glm/vec4.hpp>
#include <glm/detail/type_mat4x4.hpp>
#include <string>

#include "Handle.hpp"
#include "LightData.hpp"
#include <vector>
#include <glm/vec2.hpp>

namespace plt
{
    class PlatformInterface;
}

namespace gfx {
    struct HUDDrawItem {
        glm::vec2 screenPosition{0.0f, 0.0f}; // Top-left position in pixel coordinates
        glm::vec2 size{0.0f, 0.0f};           // Dimensions in pixels (width, height)
        glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f}; // RGBA tint / modulation
        glm::vec4 uvBounds{0.0f, 0.0f, 1.0f, 1.0f}; // uMin, vMin, uMax, vMax
        boost::uuids::uuid textureAssetId{boost::uuids::nil_uuid()}; // Texture asset UUID
        int layerDepth{0};                    // Draw order / sorting
    };

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
        virtual void drawSkybox(uint32_t cameraIndex, const glm::vec4& color) = 0;
        virtual void drawLight(PointLightData pointLightData, const glm::mat4& transform) = 0;
        virtual void drawLight(SpotLightData spotLightData, const glm::mat4& transform) = 0;
        virtual void drawLight(DirectionalLightData directionalLightData, const glm::mat4& transform) = 0;

        // HUD Rendering commands
        virtual void submitHUDItems(const std::vector<HUDDrawItem>& items) {}
        virtual void drawHUDItem(const HUDDrawItem& item) {}

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
        virtual void invalidateThumbnail(const boost::uuids::uuid& id) {}
        virtual bool captureOffscreenImage(uint32_t cameraIndex, const std::string& outputPath, int targetWidth = 128, int targetHeight = 128) { return false; }

        virtual void beginFrame() = 0;
        virtual void renderFrame() = 0;
        virtual void endFrame() = 0;
    };
} // namespace gfx

#endif //GFX_HPP