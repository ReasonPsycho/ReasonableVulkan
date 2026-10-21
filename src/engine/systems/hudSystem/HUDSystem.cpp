#include "HUDSystem.hpp"
#include "ecs/Scene.h"
#include "Engine.h"
#include "PlatformInterface.hpp"
#include <algorithm>

namespace engine::ecs {

    HUDSystem::HUDSystem(Scene* scene) : System(scene) {}

    glm::vec2 HUDSystem::CalculateAnchoredPosition(UIAnchor anchor, const glm::vec2& offset, const glm::vec2& size, const glm::vec2& screenSize) const {
        glm::vec2 anchorPos(0.0f);

        switch (anchor) {
            case UIAnchor::TopLeft:      anchorPos = glm::vec2(0.0f, 0.0f); break;
            case UIAnchor::TopCenter:    anchorPos = glm::vec2(screenSize.x * 0.5f - size.x * 0.5f, 0.0f); break;
            case UIAnchor::TopRight:     anchorPos = glm::vec2(screenSize.x - size.x, 0.0f); break;
            case UIAnchor::CenterLeft:   anchorPos = glm::vec2(0.0f, screenSize.y * 0.5f - size.y * 0.5f); break;
            case UIAnchor::Center:       anchorPos = glm::vec2((screenSize.x - size.x) * 0.5f, (screenSize.y - size.y) * 0.5f); break;
            case UIAnchor::CenterRight:  anchorPos = glm::vec2(screenSize.x - size.x, screenSize.y * 0.5f - size.y * 0.5f); break;
            case UIAnchor::BottomLeft:   anchorPos = glm::vec2(0.0f, screenSize.y - size.y); break;
            case UIAnchor::BottomCenter: anchorPos = glm::vec2(screenSize.x * 0.5f - size.x * 0.5f, screenSize.y - size.y); break;
            case UIAnchor::BottomRight:  anchorPos = glm::vec2(screenSize.x - size.x, screenSize.y - size.y); break;
        }

        return anchorPos + offset;
    }

    glm::vec2 HUDSystem::CalculateTextDimensions(const std::string& text, float fontSize, float characterSpacing, const am::FontData* fontData) const {
        if (text.empty()) {
            return glm::vec2(0.0f, fontSize);
        }

        if (fontData && !fontData->glyphs.empty()) {
            float scale = fontSize / (fontData->lineHeight > 0.0f ? fontData->lineHeight : fontSize);
            float maxX = 0.0f;
            float currentX = 0.0f;
            int lineCount = 1;

            for (size_t i = 0; i < text.length(); ++i) {
                char c = text[i];
                if (c == '\n') {
                    maxX = std::max(maxX, currentX);
                    currentX = 0.0f;
                    lineCount++;
                    continue;
                }

                int charCode = static_cast<unsigned char>(c);
                const auto* glyph = fontData->getGlyph(charCode);
                float advance = (glyph ? glyph->xadvance : (fontSize * 0.5f / scale)) + characterSpacing;
                if (i + 1 < text.length() && text[i + 1] != '\n') {
                    char nextC = text[i + 1];
                    advance += fontData->getKerning(charCode, static_cast<unsigned char>(nextC));
                }
                currentX += advance * scale;
            }
            maxX = std::max(maxX, currentX);
            float totalHeight = static_cast<float>(lineCount) * fontSize;
            return glm::vec2(maxX, totalHeight);
        }

        float charWidth = fontSize * 0.5f;
        float totalWidth = static_cast<float>(text.length()) * charWidth + static_cast<float>(text.length() - 1) * characterSpacing;
        return glm::vec2(totalWidth, fontSize);
    }

    Entity HUDSystem::GetElementAt(float screenX, float screenY, float screenWidth, float screenHeight) const {
        if (!scene || screenWidth <= 0.0f || screenHeight <= 0.0f) {
            return MAX_ENTITIES;
        }

        struct Candidate {
            Entity entity;
            int layerDepth;
        };
        std::vector<Candidate> candidates;

        for (auto [entity, hudElem] : scene->GetActiveView<HUDElementComponent>()) {
            if (!hudElem.isVisible || !hudElem.isClickable) {
                continue;
            }

            glm::vec2 anchoredPos = CalculateAnchoredPosition(
                static_cast<UIAnchor>(hudElem.anchor),
                hudElem.offset,
                hudElem.size,
                glm::vec2(screenWidth, screenHeight)
            );

            if (screenX >= anchoredPos.x && screenX <= anchoredPos.x + hudElem.size.x &&
                screenY >= anchoredPos.y && screenY <= anchoredPos.y + hudElem.size.y) {
                candidates.push_back({entity, hudElem.layerDepth});
            }
        }

        if (candidates.empty()) {
            return MAX_ENTITIES;
        }

        std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
            return a.layerDepth > b.layerDepth;
        });

        return candidates.front().entity;
    }

    Entity HUDSystem::GetElementAt(const glm::vec2& mousePos, const glm::vec2& screenSize) const {
        return GetElementAt(mousePos.x, mousePos.y, screenSize.x, screenSize.y);
    }

    bool HUDSystem::HandleClick(float screenX, float screenY, float screenWidth, float screenHeight, int mouseButton) {
        if (!scene || screenWidth <= 0.0f || screenHeight <= 0.0f) {
            return false;
        }

        Entity hitEntity = GetElementAt(screenX, screenY, screenWidth, screenHeight);
        if (hitEntity == MAX_ENTITIES) {
            return false;
        }

        events::HUDClickEvent clickEvent{
            .entity = hitEntity,
            .mousePosition = glm::vec2(screenX, screenY),
            .mouseButton = mouseButton
        };

        scene->PublishEvent(clickEvent);
        return true;
    }

    bool HUDSystem::HandleClick(const glm::vec2& mousePos, const glm::vec2& screenSize, int mouseButton) {
        return HandleClick(mousePos.x, mousePos.y, screenSize.x, screenSize.y, mouseButton);
    }

    void HUDSystem::Update(float deltaTime) {
        if (!scene || !scene->engine.graphicsEngine || scene->engine.minimized) {
            return;
        }

        int width = 0;
        int height = 0;
#ifdef ENABLE_IMGUI
        glm::uvec2 extent = scene->engine.graphicsEngine->getExtent();
        width = static_cast<int>(extent.x);
        height = static_cast<int>(extent.y);
#else
        if (scene->engine.platform) {
            scene->engine.platform->GetWindowSize(width, height);
        }
#endif
        if (width <= 0 || height <= 0) {
            return;
        }

        glm::vec2 screenSize(static_cast<float>(width), static_cast<float>(height));
        std::vector<gfx::HUDDrawItem> drawItems;

        for (auto [entity, hud] : scene->GetActiveView<HUDElementComponent>()) {
            if (!hud.isVisible) continue;

            gfx::HUDDrawItem item;
            UIAnchor anchor = static_cast<UIAnchor>(hud.anchor);
            item.screenPosition = CalculateAnchoredPosition(anchor, hud.offset, hud.size, screenSize);
            item.size = hud.size;
            item.color = hud.color;
            item.uvBounds = hud.uvBounds;
            item.textureAssetId = hud.textureUuid;
            item.layerDepth = hud.layerDepth;

            drawItems.push_back(item);
        }

        for (auto [entity, hudText] : scene->GetActiveView<HUDTextComponent>()) {
            if (!hudText.isVisible || hudText.text.empty()) continue;

            const am::FontData* fontData = nullptr;
            if (hudText.fontUuid != boost::uuids::nil_uuid() && scene->engine.assetManagerInterface) {
                fontData = scene->engine.assetManagerInterface->getAssetData<am::FontData>(hudText.fontUuid);
            }

            glm::vec2 textSize = CalculateTextDimensions(hudText.text, hudText.fontSize, hudText.characterSpacing, fontData);
            UIAnchor anchor = static_cast<UIAnchor>(hudText.anchor);
            glm::vec2 basePos = CalculateAnchoredPosition(anchor, hudText.offset, textSize, screenSize);

            boost::uuids::uuid targetTextureUuid = hudText.fontTextureUuid;
            if (targetTextureUuid == boost::uuids::nil_uuid() && fontData) {
                targetTextureUuid = fontData->atlasTextureUuid;
            }

            if (fontData && !fontData->glyphs.empty()) {
                float scale = hudText.fontSize / (fontData->lineHeight > 0.0f ? fontData->lineHeight : hudText.fontSize);
                glm::vec2 cursor = basePos;

                for (size_t i = 0; i < hudText.text.length(); ++i) {
                    char c = hudText.text[i];
                    if (c == '\n') {
                        cursor.x = basePos.x;
                        cursor.y += hudText.fontSize * hudText.lineSpacing;
                        continue;
                    }

                    int charCode = static_cast<unsigned char>(c);
                    const auto* glyph = fontData->getGlyph(charCode);
                    if (glyph) {
                        if (glyph->width > 0.0f && glyph->height > 0.0f) {
                            gfx::HUDDrawItem item;
                            item.screenPosition = cursor + glm::vec2(glyph->xoffset, glyph->yoffset) * scale;
                            item.size = glm::vec2(glyph->width, glyph->height) * scale;
                            item.color = hudText.color;
                            item.uvBounds = glm::vec4(glyph->uvMin.x, glyph->uvMin.y, glyph->uvMax.x, glyph->uvMax.y);
                            item.textureAssetId = targetTextureUuid;
                            item.layerDepth = hudText.layerDepth;
                            drawItems.push_back(item);
                        }
                        float advance = glyph->xadvance + hudText.characterSpacing;
                        if (i + 1 < hudText.text.length() && hudText.text[i + 1] != '\n') {
                            char nextC = hudText.text[i + 1];
                            advance += fontData->getKerning(charCode, static_cast<unsigned char>(nextC));
                        }
                        cursor.x += advance * scale;
                    } else {
                        cursor.x += (hudText.fontSize * 0.5f + hudText.characterSpacing);
                    }
                }
            } else if (targetTextureUuid != boost::uuids::nil_uuid()) {
                float charWidth = hudText.fontSize * 0.5f;
                for (size_t i = 0; i < hudText.text.length(); ++i) {
                    unsigned char c = static_cast<unsigned char>(hudText.text[i]);
                    float uMin = (c % 16) / 16.0f;
                    float vMin = (c / 16) / 16.0f;
                    float uMax = uMin + 1.0f / 16.0f;
                    float vMax = vMin + 1.0f / 16.0f;
                    glm::vec2 charPos = basePos + glm::vec2(static_cast<float>(i) * (charWidth + hudText.characterSpacing), 0.0f);

                    gfx::HUDDrawItem item;
                    item.screenPosition = charPos;
                    item.size = glm::vec2(charWidth, hudText.fontSize);
                    item.color = hudText.color;
                    item.uvBounds = glm::vec4(uMin, vMin, uMax, vMax);
                    item.textureAssetId = targetTextureUuid;
                    item.layerDepth = hudText.layerDepth;
                    drawItems.push_back(item);
                }
            } else {
                gfx::HUDDrawItem item;
                item.screenPosition = basePos;
                item.size = textSize;
                item.color = hudText.color;
                item.uvBounds = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
                item.textureAssetId = boost::uuids::nil_uuid();
                item.layerDepth = hudText.layerDepth;
                drawItems.push_back(item);
            }
        }

        std::sort(drawItems.begin(), drawItems.end(), [](const gfx::HUDDrawItem& a, const gfx::HUDDrawItem& b) {
            return a.layerDepth < b.layerDepth;
        });

        scene->engine.graphicsEngine->submitHUDItems(drawItems);
    }

} // namespace engine::ecs
