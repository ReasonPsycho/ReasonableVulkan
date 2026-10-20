#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/vec2.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace am {

    struct GlyphMetrics {
        int id{0};                  // Character code (e.g. ASCII / Unicode)
        float x{0.0f};              // Pixel X in atlas
        float y{0.0f};              // Pixel Y in atlas
        float width{0.0f};          // Pixel width in atlas
        float height{0.0f};         // Pixel height in atlas
        float xoffset{0.0f};        // Horizontal offset relative to cursor
        float yoffset{0.0f};        // Vertical offset relative to baseline / cursor
        float xadvance{0.0f};       // Cursor advance distance
        int page{0};                // Atlas page index
        glm::vec2 uvMin{0.0f, 0.0f};// Normalized UV top-left
        glm::vec2 uvMax{1.0f, 1.0f};// Normalized UV bottom-right
    };

    struct FontData {
        std::string fontName{""};
        float fontSize{32.0f};
        float lineHeight{32.0f};
        float baseLine{24.0f};
        float scaleW{512.0f};
        float scaleH{512.0f};
        int pagesCount{1};
        std::vector<std::string> pageFileNames;
        boost::uuids::uuid atlasTextureUuid{boost::uuids::nil_uuid()};
        std::unordered_map<int, GlyphMetrics> glyphs;
        std::unordered_map<uint64_t, float> kernings; // (first << 32) | second -> amount

        [[nodiscard]] float getKerning(int first, int second) const {
            uint64_t key = (static_cast<uint64_t>(first) << 32) | static_cast<uint32_t>(second);
            auto it = kernings.find(key);
            if (it != kernings.end()) return it->second;
            return 0.0f;
        }

        [[nodiscard]] const GlyphMetrics* getGlyph(int charCode) const {
            auto it = glyphs.find(charCode);
            if (it != glyphs.end()) return &it->second;
            return nullptr;
        }
    };

} // namespace am
