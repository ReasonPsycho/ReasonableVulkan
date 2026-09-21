#pragma once
#include <cstdint>
#include <functional>

namespace gfx {

    template<typename Tag>
    struct Handle {
        static constexpr uint32_t INVALID_INDEX = 0x000FFFFF; // 20 bits for index (~1M resources)
        static constexpr uint32_t INVALID_GEN   = 0x00000FFF; // 12 bits for generation (4096 generations)

        uint32_t index      : 20;
        uint32_t generation : 12;

        constexpr Handle()
            : index(INVALID_INDEX), generation(INVALID_GEN) {}

        constexpr Handle(uint32_t idx, uint32_t gen)
            : index(idx & 0x000FFFFF), generation(gen & 0x00000FFF) {}

        [[nodiscard]] constexpr bool isValid() const {
            return index != INVALID_INDEX;
        }

        constexpr bool operator==(const Handle& other) const {
            return index == other.index && generation == other.generation;
        }

        constexpr bool operator!=(const Handle& other) const {
            return !(*this == other);
        }

        constexpr bool operator<(const Handle& other) const {
            if (index != other.index) return index < other.index;
            return generation < other.generation;
        }

        static constexpr Handle invalid() {
            return Handle{};
        }
    };

    // Strongly-typed handle tag aliases
    struct ModelTag {};
    struct ShaderProgramTag {};
    struct ShaderTag {};
    struct TextureTag {};
    struct MaterialTag {};
    struct MeshTag {};

    using ModelHandle         = Handle<ModelTag>;
    using ShaderProgramHandle = Handle<ShaderProgramTag>;
    using ShaderHandle        = Handle<ShaderTag>;
    using TextureHandle       = Handle<TextureTag>;
    using MaterialHandle      = Handle<MaterialTag>;
    using MeshHandle          = Handle<MeshTag>;

} // namespace gfx

namespace std {
    template<typename Tag>
    struct hash<gfx::Handle<Tag>> {
        size_t operator()(const gfx::Handle<Tag>& h) const noexcept {
            uint32_t raw = (h.index & 0x000FFFFF) | ((h.generation & 0x00000FFF) << 20);
            return std::hash<uint32_t>{}(raw);
        }
    };
}
