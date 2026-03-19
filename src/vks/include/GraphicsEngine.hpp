
#ifndef GFX_HPP
#define GFX_HPP
#include <boost/mp11/integral.hpp>
#include <boost/uuid/uuid.hpp>
#include <glm/fwd.hpp>
#include <glm/detail/type_mat4x4.hpp>

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

        virtual void setCameraData(uint32_t cameraIndex, const glm::mat4& projection, const glm::mat4& view, const glm::vec3 cameraPos) = 0;
        virtual void drawModel(boost::uuids::uuid modelId, boost::uuids::uuid shaderId, const glm::mat4& transform) = 0;
        virtual void drawSkybox(boost::uuids::uuid textureId, boost::uuids::uuid shaderId) = 0;
        virtual void drawLight(PointLightData pointLightData, const glm::mat4& transform) = 0;
        virtual void drawLight(SpotLightData spotLightData, const glm::mat4& transform) = 0;
        virtual void drawLight(DirectionalLightData directionalLightData, const glm::mat4& transform) = 0;
        virtual void loadModel(boost::uuids::uuid uuid) = 0;
        virtual void loadShader(boost::uuids::uuid uuid) = 0;
        virtual void loadTexture(boost::uuids::uuid uuid) = 0;

        virtual void beginFrame() = 0;
        virtual void renderFrame() = 0;
        virtual void endFrame() = 0;
    };
} // namespace gfx

#endif //GFX_HPP