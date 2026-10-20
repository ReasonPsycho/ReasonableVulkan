#include "FontAsset.hpp"
#include "../../AssetManager.hpp"
#include "../../JsonHelpers.hpp"
#include "stb_image_write.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <filesystem>
#include <boost/uuid/uuid_io.hpp>
#include <spdlog/spdlog.h>

namespace am {
    static std::unordered_map<std::string, std::string> parseFntKeyValuePairs(const std::string &line) {
        std::unordered_map<std::string, std::string> result;
        size_t i = 0;
        while (i < line.size()) {
            while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
            if (i >= line.size()) break;

            size_t keyStart = i;
            while (i < line.size() && line[i] != '=' && !std::isspace(static_cast<unsigned char>(line[i]))) ++i;
            std::string key = line.substr(keyStart, i - keyStart);

            if (i < line.size() && line[i] == '=') {
                ++i; // skip '='
                std::string val;
                if (i < line.size() && line[i] == '"') {
                    ++i; // skip opening quote
                    size_t valStart = i;
                    while (i < line.size() && line[i] != '"') ++i;
                    val = line.substr(valStart, i - valStart);
                    if (i < line.size() && line[i] == '"') ++i; // skip closing quote
                } else {
                    size_t valStart = i;
                    while (i < line.size() && !std::isspace(static_cast<unsigned char>(line[i]))) ++i;
                    val = line.substr(valStart, i - valStart);
                }
                result[key] = val;
            }
        }
        return result;
    }

    FontAsset::FontAsset(const boost::uuids::uuid &id, std::string path) : Asset(id, path) {
    }

    FontAsset::FontAsset(const boost::uuids::uuid &id, ImportContext assetFactoryData)
        : Asset(id, assetFactoryData) {
        if (!assetFactoryData.importPath.empty()) {
            std::string ext = std::filesystem::path(assetFactoryData.importPath).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".fnt") {
                loadFromFnt(assetFactoryData.importPath);
            } else if (ext == ".json" || ext == ".font") {
                loadFromJson(assetFactoryData.importPath);
            } else if (ext == ".ttf" || ext == ".otf" || ext == ".ttc" || ext == ".woff" || ext == ".woff2") {
                loadWithFreeType(assetFactoryData.importPath);
            } else {
                if (!loadWithFreeType(assetFactoryData.importPath)) {
                    loadFromFnt(assetFactoryData.importPath);
                }
            }
        }
    }

    FontAsset::FontAsset(const std::string &path, AssetFormat format) : Asset(path, format) {
        if (format == AssetFormat::Json) {
            loadFromJson(path);
        } else {
            std::string ext = std::filesystem::path(path).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".ttf" || ext == ".otf" || ext == ".ttc" || ext == ".woff" || ext == ".woff2") {
                loadWithFreeType(path);
            } else if (ext == ".fnt") {
                loadFromFnt(path);
            } else {
                if (!loadWithFreeType(path)) {
                    loadFromFnt(path);
                }
            }
        }
    }

    AssetType FontAsset::getType() const {
        return AssetType::Font;
    }

    size_t FontAsset::calculateContentHash() const {
        size_t hash = 0;
        hash ^= std::hash<std::string>{}(data.fontName);
        hash ^= std::hash<float>{}(data.fontSize) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<float>{}(data.lineHeight) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<size_t>{}(data.glyphs.size()) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        return hash;
    }

    void FontAsset::loadFromFnt(const std::string &path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            spdlog::error("FontAsset: Failed to open font file: {}", path);
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Trim leading spaces
            size_t start = 0;
            while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start]))) ++start;
            if (start >= line.size()) continue;

            std::string trimmed = line.substr(start);
            auto pairs = parseFntKeyValuePairs(trimmed);

            if (trimmed.rfind("info", 0) == 0) {
                if (pairs.count("face")) data.fontName = pairs["face"];
                if (pairs.count("size")) {
                    try { data.fontSize = std::stof(pairs["size"]); } catch (...) {
                    }
                }
            } else if (trimmed.rfind("common", 0) == 0) {
                if (pairs.count("lineHeight")) {
                    try { data.lineHeight = std::stof(pairs["lineHeight"]); } catch (...) {
                    }
                }
                if (pairs.count("base")) {
                    try { data.baseLine = std::stof(pairs["base"]); } catch (...) {
                    }
                }
                if (pairs.count("scaleW")) {
                    try { data.scaleW = std::stof(pairs["scaleW"]); } catch (...) {
                    }
                }
                if (pairs.count("scaleH")) {
                    try { data.scaleH = std::stof(pairs["scaleH"]); } catch (...) {
                    }
                }
                if (pairs.count("pages")) {
                    try { data.pagesCount = std::stoi(pairs["pages"]); } catch (...) {
                    }
                }
            } else if (trimmed.rfind("page", 0) == 0) {
                if (pairs.count("file")) {
                    data.pageFileNames.push_back(pairs["file"]);
                }
            } else if (trimmed.rfind("char ", 0) == 0) {
                if (pairs.count("id")) {
                    GlyphMetrics glyph;
                    try {
                        glyph.id = std::stoi(pairs["id"]);
                        if (pairs.count("x")) glyph.x = std::stof(pairs["x"]);
                        if (pairs.count("y")) glyph.y = std::stof(pairs["y"]);
                        if (pairs.count("width")) glyph.width = std::stof(pairs["width"]);
                        if (pairs.count("height")) glyph.height = std::stof(pairs["height"]);
                        if (pairs.count("xoffset")) glyph.xoffset = std::stof(pairs["xoffset"]);
                        if (pairs.count("yoffset")) glyph.yoffset = std::stof(pairs["yoffset"]);
                        if (pairs.count("xadvance")) glyph.xadvance = std::stof(pairs["xadvance"]);
                        if (pairs.count("page")) glyph.page = std::stoi(pairs["page"]);

                        float sW = data.scaleW > 0.0f ? data.scaleW : 1.0f;
                        float sH = data.scaleH > 0.0f ? data.scaleH : 1.0f;
                        glyph.uvMin = glm::vec2(glyph.x / sW, glyph.y / sH);
                        glyph.uvMax = glm::vec2((glyph.x + glyph.width) / sW, (glyph.y + glyph.height) / sH);

                        data.glyphs[glyph.id] = glyph;
                    } catch (...) {
                    }
                }
            } else if (trimmed.rfind("kerning ", 0) == 0) {
                if (pairs.count("first") && pairs.count("second") && pairs.count("amount")) {
                    try {
                        int first = std::stoi(pairs["first"]);
                        int second = std::stoi(pairs["second"]);
                        float amount = std::stof(pairs["amount"]);
                        uint64_t key = (static_cast<uint64_t>(first) << 32) | static_cast<uint32_t>(second);
                        data.kernings[key] = amount;
                    } catch (...) {
                    }
                }
            }
        }

        // Try to automatically link atlas texture if page file is specified
        if (!data.pageFileNames.empty()) {
            std::filesystem::path fntPath(path);
            std::filesystem::path pageTexPath = fntPath.parent_path() / data.pageFileNames[0];
            if (std::filesystem::exists(pageTexPath)) {
                auto texUuid = AssetManager::getInstance().getAssetUuidByPath(pageTexPath);
                if (texUuid.has_value()) {
                    data.atlasTextureUuid = texUuid.value();
                }
            }
        }

        spdlog::info("FontAsset: Loaded font '{}' with {} glyphs from {}", data.fontName, data.glyphs.size(), path);
    }

    void FontAsset::loadFromJson(const std::string &path) {
        rapidjson::Document doc;
        if (!loadJsonFromFile(path, doc)) {
            spdlog::error("FontAsset: Failed to load JSON from: {}", path);
            return;
        }

        if (doc.HasMember("uuid") && doc["uuid"].IsString()) {
            id = boost::uuids::string_generator()(doc["uuid"].GetString());
        }
        if (doc.HasMember("fontName") && doc["fontName"].IsString()) {
            data.fontName = doc["fontName"].GetString();
        }
        if (doc.HasMember("fontSize") && doc["fontSize"].IsNumber()) {
            data.fontSize = doc["fontSize"].GetFloat();
        }
        if (doc.HasMember("lineHeight") && doc["lineHeight"].IsNumber()) {
            data.lineHeight = doc["lineHeight"].GetFloat();
        }
        if (doc.HasMember("baseLine") && doc["baseLine"].IsNumber()) {
            data.baseLine = doc["baseLine"].GetFloat();
        }
        if (doc.HasMember("scaleW") && doc["scaleW"].IsNumber()) {
            data.scaleW = doc["scaleW"].GetFloat();
        }
        if (doc.HasMember("scaleH") && doc["scaleH"].IsNumber()) {
            data.scaleH = doc["scaleH"].GetFloat();
        }
        if (doc.HasMember("atlasTextureUuid") && doc["atlasTextureUuid"].IsString()) {
            try {
                data.atlasTextureUuid = boost::uuids::string_generator()(doc["atlasTextureUuid"].GetString());
            } catch (...) {
            }
        }

        if (doc.HasMember("glyphs") && doc["glyphs"].IsArray()) {
            const auto &arr = doc["glyphs"];
            for (rapidjson::SizeType i = 0; i < arr.Size(); ++i) {
                const auto &item = arr[i];
                if (!item.IsObject() || !item.HasMember("id")) continue;
                GlyphMetrics g;
                g.id = item["id"].GetInt();
                if (item.HasMember("x")) g.x = item["x"].GetFloat();
                if (item.HasMember("y")) g.y = item["y"].GetFloat();
                if (item.HasMember("width")) g.width = item["width"].GetFloat();
                if (item.HasMember("height")) g.height = item["height"].GetFloat();
                if (item.HasMember("xoffset")) g.xoffset = item["xoffset"].GetFloat();
                if (item.HasMember("yoffset")) g.yoffset = item["yoffset"].GetFloat();
                if (item.HasMember("xadvance")) g.xadvance = item["xadvance"].GetFloat();
                if (item.HasMember("page")) g.page = item["page"].GetInt();

                float sW = data.scaleW > 0.0f ? data.scaleW : 1.0f;
                float sH = data.scaleH > 0.0f ? data.scaleH : 1.0f;
                g.uvMin = glm::vec2(g.x / sW, g.y / sH);
                g.uvMax = glm::vec2((g.x + g.width) / sW, (g.y + g.height) / sH);

                data.glyphs[g.id] = g;
            }
        }
    }

    void FontAsset::SaveAssetToJson(rapidjson::Document &document) {
        auto &allocator = document.GetAllocator();
        if (!document.IsObject()) document.SetObject();

        document.AddMember("uuid", rapidjson::Value(boost::uuids::to_string(id).c_str(), allocator), allocator);
        document.AddMember("type", "Font", allocator);
        document.AddMember("fontName", rapidjson::Value(data.fontName.c_str(), allocator), allocator);
        document.AddMember("fontSize", data.fontSize, allocator);
        document.AddMember("lineHeight", data.lineHeight, allocator);
        document.AddMember("baseLine", data.baseLine, allocator);
        document.AddMember("scaleW", data.scaleW, allocator);
        document.AddMember("scaleH", data.scaleH, allocator);
        document.AddMember("atlasTextureUuid",
                           rapidjson::Value(boost::uuids::to_string(data.atlasTextureUuid).c_str(), allocator),
                           allocator);

        rapidjson::Value glyphsArray(rapidjson::kArrayType);
        for (const auto &[gId, glyph]: data.glyphs) {
            rapidjson::Value gObj(rapidjson::kObjectType);
            gObj.AddMember("id", glyph.id, allocator);
            gObj.AddMember("x", glyph.x, allocator);
            gObj.AddMember("y", glyph.y, allocator);
            gObj.AddMember("width", glyph.width, allocator);
            gObj.AddMember("height", glyph.height, allocator);
            gObj.AddMember("xoffset", glyph.xoffset, allocator);
            gObj.AddMember("yoffset", glyph.yoffset, allocator);
            gObj.AddMember("xadvance", glyph.xadvance, allocator);
            gObj.AddMember("page", glyph.page, allocator);
            glyphsArray.PushBack(gObj, allocator);
        }
        document.AddMember("glyphs", glyphsArray, allocator);
    }

    void FontAsset::SaveAssetMetadata(rapidjson::Document &document) {
        auto &allocator = document.GetAllocator();
        document.AddMember("fontName", rapidjson::Value(data.fontName.c_str(), allocator), allocator);
        document.AddMember("atlasTextureUuid",
                           rapidjson::Value(boost::uuids::to_string(data.atlasTextureUuid).c_str(), allocator),
                           allocator);
    }

    void FontAsset::LoadAssetMetadata(rapidjson::Document &document) {
        if (document.HasMember("fontName") && document["fontName"].IsString()) {
            data.fontName = document["fontName"].GetString();
        }
        if (document.HasMember("atlasTextureUuid") && document["atlasTextureUuid"].IsString()) {
            try {
                data.atlasTextureUuid = boost::uuids::string_generator()(document["atlasTextureUuid"].GetString());
            } catch (...) {
            }
        }
    }

    bool FontAsset::loadWithFreeType(const std::string &path, float targetFontSize) {
        FT_Library ft = nullptr;
        if (FT_Init_FreeType(&ft)) {
            spdlog::error("FontAsset: Failed to initialize FreeType library");
            return false;
        }

        FT_Face face = nullptr;
        if (FT_New_Face(ft, path.c_str(), 0, &face)) {
            spdlog::error("FontAsset: Failed to load font face with FreeType from: {}", path);
            FT_Done_FreeType(ft);
            return false;
        }

        if (FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(targetFontSize))) {
            spdlog::warn("FontAsset: Failed to set pixel size {} for font: {}", targetFontSize, path);
        }

        data.fontName = face->family_name ? face->family_name : "FreeTypeFont";
        if (face->style_name && std::string(face->style_name) != "Regular") {
            data.fontName += " ";
            data.fontName += face->style_name;
        }
        data.fontSize = targetFontSize;
        data.lineHeight = static_cast<float>(face->size->metrics.height >> 6);
        data.baseLine = static_cast<float>(face->size->metrics.ascender >> 6);
        if (data.lineHeight <= 0.0f) data.lineHeight = targetFontSize * 1.25f;
        if (data.baseLine <= 0.0f) data.baseLine = targetFontSize * 0.8f;

        // Collect character codes to rasterize (ASCII 32..126 + extended ASCII if available)
        std::vector<int> charCodes;
        for (int c = 32; c <= 126; ++c) {
            charCodes.push_back(c);
        }
        for (int c = 160; c <= 255; ++c) {
            if (FT_Get_Char_Index(face, c) != 0) {
                charCodes.push_back(c);
            }
        }

        int atlasWidth = (targetFontSize > 36.0f) ? 1024 : 512;
        int atlasHeight = (targetFontSize > 36.0f) ? 1024 : 512;
        const int padding = 2;

        std::vector<uint8_t> atlasPixels(atlasWidth * atlasHeight * 4, 0);
        data.glyphs.clear();
        data.scaleW = static_cast<float>(atlasWidth);
        data.scaleH = static_cast<float>(atlasHeight);
        data.pagesCount = 1;

        int curX = padding;
        int curY = padding;
        int rowHeight = 0;

        for (int charCode: charCodes) {
            if (FT_Load_Char(face, charCode, FT_LOAD_RENDER)) {
                continue;
            }

            FT_GlyphSlot slot = face->glyph;
            int w = slot->bitmap.width;
            int h = slot->bitmap.rows;

            if (curX + w + padding >= atlasWidth) {
                curX = padding;
                curY += rowHeight + padding;
                rowHeight = 0;
            }

            if (curY + h + padding >= atlasHeight) {
                int newHeight = atlasHeight * 2;
                atlasPixels.resize(atlasWidth * newHeight * 4, 0);
                atlasHeight = newHeight;
                data.scaleH = static_cast<float>(atlasHeight);
            }

            if (w > 0 && h > 0 && slot->bitmap.buffer) {
                for (int row = 0; row < h; ++row) {
                    for (int col = 0; col < w; ++col) {
                        uint8_t alpha = slot->bitmap.buffer[row * slot->bitmap.pitch + col];
                        size_t dstIdx = ((curY + row) * atlasWidth + (curX + col)) * 4;
                        atlasPixels[dstIdx + 0] = 255;
                        atlasPixels[dstIdx + 1] = 255;
                        atlasPixels[dstIdx + 2] = 255;
                        atlasPixels[dstIdx + 3] = alpha;
                    }
                }
            }

            GlyphMetrics glyph;
            glyph.id = charCode;
            glyph.x = static_cast<float>(curX);
            glyph.y = static_cast<float>(curY);
            glyph.width = static_cast<float>(w);
            glyph.height = static_cast<float>(h);
            glyph.xoffset = static_cast<float>(slot->bitmap_left);
            glyph.yoffset = data.baseLine - static_cast<float>(slot->bitmap_top);
            glyph.xadvance = static_cast<float>(slot->advance.x >> 6);
            glyph.page = 0;
            glyph.uvMin = glm::vec2(glyph.x / data.scaleW, glyph.y / data.scaleH);
            glyph.uvMax = glm::vec2((glyph.x + glyph.width) / data.scaleW, (glyph.y + glyph.height) / data.scaleH);

            data.glyphs[charCode] = glyph;

            curX += w + padding;
            rowHeight = std::max(rowHeight, h);
        }

        // Recompute normalized UVs in case atlasHeight was expanded
        for (auto &[gId, g]: data.glyphs) {
            g.uvMin = glm::vec2(g.x / data.scaleW, g.y / data.scaleH);
            g.uvMax = glm::vec2((g.x + g.width) / data.scaleW, (g.y + g.height) / data.scaleH);
        }

        // Extract Kerning table
        data.kernings.clear();
        if (FT_HAS_KERNING(face)) {
            for (const auto &[c1, _]: data.glyphs) {
                FT_UInt idx1 = FT_Get_Char_Index(face, c1);
                if (!idx1) continue;
                for (const auto &[c2, __]: data.glyphs) {
                    FT_UInt idx2 = FT_Get_Char_Index(face, c2);
                    if (!idx2) continue;
                    FT_Vector kernVec;
                    if (FT_Get_Kerning(face, idx1, idx2, FT_KERNING_DEFAULT, &kernVec) == 0 && kernVec.x != 0) {
                        float amount = static_cast<float>(kernVec.x >> 6);
                        uint64_t key = (static_cast<uint64_t>(c1) << 32) | static_cast<uint32_t>(c2);
                        data.kernings[key] = amount;
                    }
                }
            }
        }

        FT_Done_Face(face);
        FT_Done_FreeType(ft);

        // Generate and save font atlas image
        std::filesystem::path fontPath(path);
        std::filesystem::path atlasPath = fontPath.parent_path() / (fontPath.stem().string() + "_atlas.png");
        data.pageFileNames.clear();
        data.pageFileNames.push_back(atlasPath.filename().string());

        std::error_code ec;
        if (!std::filesystem::exists(atlasPath.parent_path(), ec)) {
            std::filesystem::create_directories(atlasPath.parent_path(), ec);
        }

        int writeRes = stbi_write_png(atlasPath.string().c_str(), atlasWidth, atlasHeight, 4, atlasPixels.data(),
                                      atlasWidth * 4);
        if (writeRes) {
            spdlog::info("FontAsset: Generated font atlas texture: {}", atlasPath.string());
            auto texUuid = AssetManager::getInstance().getAssetUuidByPath(atlasPath);
            if (!texUuid.has_value()) {
                texUuid = AssetManager::getInstance().registerAsset(atlasPath.string());
            }
            if (texUuid.has_value()) {
                data.atlasTextureUuid = texUuid.value();
            }
        } else {
            spdlog::warn("FontAsset: Failed to write font atlas image to {}", atlasPath.string());
        }

        spdlog::info("FontAsset: Loaded FreeType font '{}' with {} glyphs from {}", data.fontName, data.glyphs.size(),
                     path);
        return true;
    }
} // namespace am
