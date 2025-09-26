//
// Created by redkc on 06.05.2025.
//

#ifndef GFX_HPP
#define GFX_HPP
#include <boost/mp11/integral.hpp>
#include <boost/uuid/uuid.hpp>
#include <glm/fwd.hpp>

namespace gfx {
    class GraphicsEngine {
    protected:
        GraphicsEngine() = default;

    public:
        virtual ~GraphicsEngine() = default;

        virtual void Init();

        virtual void drawModel(boost::uuids::uuid uuid, const glm::mat4& transform) = 0;
        virtual void loadModel(boost::uuids::uuid uuid) = 0;
        virtual void loadShader(boost::uuids::uuid uuid) = 0;

        virtual void renderFrame() = 0;
        virtual void render() = 0;
        virtual void endFrame() = 0;
    };
} // namespace gfx

#endif //GFX_HPP
