
#ifndef GFX_HPP
#define GFX_HPP
#include <boost/mp11/integral.hpp>
#include <boost/uuid/uuid.hpp>
#include <glm/fwd.hpp>
#include <glm/detail/type_mat4x4.hpp>

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

        virtual void setCameraData(const glm::mat4& projection, const glm::mat4& view) = 0;
        virtual void drawModel(boost::uuids::uuid uuid, const glm::mat4& transform) = 0;
        virtual void drawLight(, const glm::mat4& transform) = 0;
        virtual void loadModel(boost::uuids::uuid uuid) = 0;
        virtual void loadShader(boost::uuids::uuid uuid) = 0;

        virtual void beginFrame() = 0;
        virtual void renderFrame() = 0;
        virtual void endFrame() = 0;
    };
} // namespace gfx

#endif //GFX_HPP