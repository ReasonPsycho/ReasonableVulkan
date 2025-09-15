//
// Created by redkc on 06.05.2025.
//

#ifndef GFX_HPP
#define GFX_HPP

namespace gfx {
    class GraphicsEngine {
    protected:
        GraphicsEngine() = default;

    public:
        virtual ~GraphicsEngine() = default;

        virtual void Init();

        virtual void submitMesh(MeshHandle mesh, const glm::mat4& transform) = 0;

        virtual void renderFrame() = 0;

        virtual void render() = 0;

        virtual void endFrame() = 0;
    };
} // namespace gfx

#endif //GFX_HPP
