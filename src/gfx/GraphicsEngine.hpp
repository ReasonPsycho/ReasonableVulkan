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

        virtual void beginFrame() = 0;

        virtual void render() = 0;

        virtual void endFrame() = 0;
    };
} // namespace gfx

#endif //GFX_HPP
