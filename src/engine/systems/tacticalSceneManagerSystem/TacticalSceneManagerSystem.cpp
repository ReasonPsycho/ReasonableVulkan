#include "TacticalSceneManagerSystem.hpp"

#include <limits>
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

#ifdef ENABLE_IMGUI
#include <imgui.h>
#include "IconsFontAwesome6.h"
#endif

#include "UUIDManager.hpp"
#include "ecs/Scene.h"
#include "systems/transformSystem/componets/TransformComponent.hpp"
#include "systems/renderingSystem/componets/CameraComponent.hpp"
#include "systems/collisionSystem/CollisionSystem.hpp"
#include "systems/pathfindingSystem/PathfindingSystem.hpp"
#include "systems/pathfindingSystem/componets/GridFragment.hpp"
#include "systems/tacticalGridSystem/TacticalGridSystem.hpp"
#include "systems/gizmoSystem/GizmoSystem.hpp"
#include "systems/hudSystem/HUDSystem.hpp"

namespace engine::ecs {

    TacticalSceneManagerSystem::TacticalSceneManagerSystem(Scene* scene) : System(scene) {
        if (scene) {
            hudClickListenerId = scene->GetEventDispatcher().Subscribe<events::HUDClickEvent>(
                [this](const events::HUDClickEvent& ev) {
                    OnHUDClicked(ev);
                }
            );
        }
    }

    TacticalSceneManagerSystem::~TacticalSceneManagerSystem() {
        if (scene && hudClickListenerId != 0) {
            scene->GetEventDispatcher().Unsubscribe<events::HUDClickEvent>(hudClickListenerId);
            hudClickListenerId = 0;
        }
    }

    void TacticalSceneManagerSystem::OnHUDClicked(const events::HUDClickEvent& ev) {
        if (!scene || !HasSelectedEntity()) return;

        if (ev.entity == hudActionButtonEntity) {
            if (isShootModeActive) {
                ExecuteShoot();
            } else {
                ToggleShootMode();
            }
            return;
        }

        if (isShootModeActive) {
            for (const auto& card : hudTargetCards) {
                if (ev.entity == card.bgEntity && card.entityRef != MAX_ENTITIES) {
                    if (card.entityRef == targetUnitEntity) {
                        ExecuteShoot(card.entityRef);
                    } else {
                        SelectTargetUnit(card.entityRef);
                    }
                    return;
                }
            }
        }
    }

    void TacticalSceneManagerSystem::SetSelectedEntity(Entity entity) {
        if (entity != MAX_ENTITIES && scene && scene->IsEntityActive(entity)) {
            if (requirePathfindingAgent) {
                if (scene->HasComponent<PathfindingAgent>(entity)) {
                    selectedEntity = entity;
                } else {
                    selectedEntity = MAX_ENTITIES;
                }
            } else {
                selectedEntity = entity;
            }
        } else {
            selectedEntity = MAX_ENTITIES;
        }
        if (selectedEntity == MAX_ENTITIES || selectedEntity == targetUnitEntity) {
            targetUnitEntity = MAX_ENTITIES;
        }
        UpdatePawnHUD();
    }

    void TacticalSceneManagerSystem::ClearSelection() {
        selectedEntity = MAX_ENTITIES;
        isShootModeActive = false;
        targetUnitEntity = MAX_ENTITIES;
        UpdatePawnHUD();
    }

    bool TacticalSceneManagerSystem::HasSelectedEntity() const {
        return selectedEntity != MAX_ENTITIES && scene && scene->IsEntityActive(selectedEntity);
    }

    std::string TacticalSceneManagerSystem::GetSelectedEntityName() const {
        if (!HasSelectedEntity()) {
            return "";
        }
        if (scene->HasComponent<PawnComponent>(selectedEntity)) {
            return scene->GetComponent<PawnComponent>(selectedEntity).name;
        }
        return scene->GetEntityName(selectedEntity);
    }

    std::optional<float> TacticalSceneManagerSystem::GetSelectedEntityHealth() const {
        if (!HasSelectedEntity() || !scene->HasComponent<PawnComponent>(selectedEntity)) {
            return std::nullopt;
        }
        return scene->GetComponent<PawnComponent>(selectedEntity).currentHealth;
    }

    std::optional<float> TacticalSceneManagerSystem::GetSelectedEntityMaxHealth() const {
        if (!HasSelectedEntity() || !scene->HasComponent<PawnComponent>(selectedEntity)) {
            return std::nullopt;
        }
        return scene->GetComponent<PawnComponent>(selectedEntity).maxHealth;
    }

    std::optional<PawnComponent> TacticalSceneManagerSystem::GetSelectedPawnComponent() const {
        if (!HasSelectedEntity() || !scene->HasComponent<PawnComponent>(selectedEntity)) {
            return std::nullopt;
        }
        return scene->GetComponent<PawnComponent>(selectedEntity);
    }

    bool TacticalSceneManagerSystem::GetSelectedAgentInfo(std::string& outName, float& outCurrentHealth, float& outMaxHealth) const {
        if (!HasSelectedEntity()) {
            return false;
        }
        if (scene->HasComponent<PawnComponent>(selectedEntity)) {
            const auto& pawn = scene->GetComponent<PawnComponent>(selectedEntity);
            outName = pawn.name;
            outCurrentHealth = pawn.currentHealth;
            outMaxHealth = pawn.maxHealth;
        } else {
            outName = GetSelectedEntityName();
            outCurrentHealth = 0.0f;
            outMaxHealth = 0.0f;
        }
        return true;
    }

    void TacticalSceneManagerSystem::OnEntityRemoved(ComponentID componentID, std::type_index type) {
        if (selectedEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(selectedEntity)) {
            ClearSelection();
        }
        if (hudHealthBackgroundEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(hudHealthBackgroundEntity)) {
            hudHealthBackgroundEntity = MAX_ENTITIES;
        }
        if (hudHealthFillEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(hudHealthFillEntity)) {
            hudHealthFillEntity = MAX_ENTITIES;
        }
        if (hudNameTextEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(hudNameTextEntity)) {
            hudNameTextEntity = MAX_ENTITIES;
        }
        if (hudStatsTextEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(hudStatsTextEntity)) {
            hudStatsTextEntity = MAX_ENTITIES;
        }
        if (hudActionButtonEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(hudActionButtonEntity)) {
            hudActionButtonEntity = MAX_ENTITIES;
        }
        if (hudActionTextEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(hudActionTextEntity)) {
            hudActionTextEntity = MAX_ENTITIES;
        }
        if (hudTargetHeaderEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(hudTargetHeaderEntity)) {
            hudTargetHeaderEntity = MAX_ENTITIES;
        }
        if (hudFeedbackTextEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(hudFeedbackTextEntity)) {
            hudFeedbackTextEntity = MAX_ENTITIES;
        }
        for (auto& card : hudTargetCards) {
            if (card.bgEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(card.bgEntity)) card.bgEntity = MAX_ENTITIES;
            if (card.fillEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(card.fillEntity)) card.fillEntity = MAX_ENTITIES;
            if (card.textEntity != MAX_ENTITIES && scene && !scene->IsEntityActive(card.textEntity)) card.textEntity = MAX_ENTITIES;
        }
    }

    void TacticalSceneManagerSystem::CreateSelectedPawnHUDEntities(const glm::vec2& size, const glm::vec2& offset, UIAnchor anchor) {
        if (!scene) return;

        DestroySelectedPawnHUDEntities();

        // 1. Right-side Selected Pawn Status Panel (Anchor: BottomRight by default)
        hudHealthBackgroundEntity = scene->CreateEntity("HUD_SelectedPawn_HP_Background");
        HUDElementComponent bgComp;
        bgComp.anchor = static_cast<int>(anchor);
        bgComp.offset = offset;
        bgComp.size = size;
        bgComp.color = glm::vec4(0.12f, 0.15f, 0.2f, 0.9f);
        bgComp.layerDepth = hudBackgroundLayerDepth;
        bgComp.isVisible = false;
        scene->AddComponent<HUDElementComponent>(hudHealthBackgroundEntity, bgComp);

        hudHealthFillEntity = scene->CreateEntity("HUD_SelectedPawn_HP_Fill");
        HUDElementComponent fillComp;
        fillComp.anchor = static_cast<int>(anchor);
        fillComp.offset = offset;
        fillComp.size = size;
        fillComp.color = glm::vec4(0.2f, 0.8f, 0.2f, 1.0f);
        fillComp.layerDepth = hudFillLayerDepth;
        fillComp.isVisible = false;
        scene->AddComponent<HUDElementComponent>(hudHealthFillEntity, fillComp);

        hudNameTextEntity = scene->CreateEntity("HUD_SelectedPawn_Name_Text");
        HUDTextComponent textComp;
        textComp.anchor = static_cast<int>(anchor);
        textComp.offset = glm::vec2(offset.x, offset.y - 25.0f);
        textComp.fontSize = 16.0f;
        textComp.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        textComp.fontUuid = hudFontUuid;
        textComp.layerDepth = hudTextLayerDepth;
        textComp.isVisible = false;
        scene->AddComponent<HUDTextComponent>(hudNameTextEntity, textComp);

        hudStatsTextEntity = scene->CreateEntity("HUD_SelectedPawn_Stats_Text");
        HUDTextComponent statsComp;
        statsComp.anchor = static_cast<int>(anchor);
        statsComp.offset = glm::vec2(offset.x, offset.y + size.y + 4.0f);
        statsComp.fontSize = 13.0f;
        statsComp.color = glm::vec4(0.75f, 0.82f, 0.9f, 1.0f);
        statsComp.fontUuid = hudFontUuid;
        statsComp.layerDepth = hudTextLayerDepth;
        statsComp.isVisible = false;
        scene->AddComponent<HUDTextComponent>(hudStatsTextEntity, statsComp);

        // 2. Center Action Button (Shoot / Fire)
        hudActionButtonEntity = scene->CreateEntity("HUD_Tactical_Action_Button");
        HUDElementComponent actionBg;
        actionBg.anchor = static_cast<int>(UIAnchor::BottomCenter);
        actionBg.offset = glm::vec2(-80.0f, -55.0f);
        actionBg.size = glm::vec2(160.0f, 40.0f);
        actionBg.color = glm::vec4(0.65f, 0.15f, 0.15f, 0.95f);
        actionBg.layerDepth = hudBackgroundLayerDepth;
        actionBg.isVisible = false;
        actionBg.isClickable = true;
        scene->AddComponent<HUDElementComponent>(hudActionButtonEntity, actionBg);

        hudActionTextEntity = scene->CreateEntity("HUD_Tactical_Action_Text");
        HUDTextComponent actionText;
        actionText.anchor = static_cast<int>(UIAnchor::BottomCenter);
        actionText.offset = glm::vec2(-55.0f, -44.0f);
        actionText.fontSize = 16.0f;
        actionText.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        actionText.text = "SHOOT (1)";
        actionText.fontUuid = hudFontUuid;
        actionText.layerDepth = hudTextLayerDepth;
        actionText.isVisible = false;
        scene->AddComponent<HUDTextComponent>(hudActionTextEntity, actionText);

        // 3. Center Target Mode Header
        hudTargetHeaderEntity = scene->CreateEntity("HUD_Tactical_Target_Header");
        HUDTextComponent targetHeader;
        targetHeader.anchor = static_cast<int>(UIAnchor::BottomCenter);
        targetHeader.offset = glm::vec2(-160.0f, -165.0f);
        targetHeader.fontSize = 13.0f;
        targetHeader.color = glm::vec4(1.0f, 0.85f, 0.2f, 1.0f);
        targetHeader.text = "TARGETS (Click or Tab/Arrows to cycle)";
        targetHeader.fontUuid = hudFontUuid;
        targetHeader.layerDepth = hudTextLayerDepth;
        targetHeader.isVisible = false;
        scene->AddComponent<HUDTextComponent>(hudTargetHeaderEntity, targetHeader);

        // 4. Center Target Cards (Pool of 8 card entities)
        hudTargetCards.clear();
        for (int i = 0; i < 8; ++i) {
            HUDTargetCardEntities card;
            card.bgEntity = scene->CreateEntity("HUD_Target_Card_Bg_" + std::to_string(i));
            HUDElementComponent cBg;
            cBg.anchor = static_cast<int>(UIAnchor::BottomCenter);
            cBg.size = glm::vec2(105.0f, 42.0f);
            cBg.color = glm::vec4(0.12f, 0.15f, 0.2f, 0.9f);
            cBg.layerDepth = hudBackgroundLayerDepth;
            cBg.isVisible = false;
            cBg.isClickable = true;
            scene->AddComponent<HUDElementComponent>(card.bgEntity, cBg);

            card.fillEntity = scene->CreateEntity("HUD_Target_Card_Fill_" + std::to_string(i));
            HUDElementComponent cFill;
            cFill.anchor = static_cast<int>(UIAnchor::BottomCenter);
            cFill.size = glm::vec2(95.0f, 4.0f);
            cFill.color = glm::vec4(0.85f, 0.2f, 0.2f, 1.0f);
            cFill.layerDepth = hudFillLayerDepth;
            cFill.isVisible = false;
            scene->AddComponent<HUDElementComponent>(card.fillEntity, cFill);

            card.textEntity = scene->CreateEntity("HUD_Target_Card_Text_" + std::to_string(i));
            HUDTextComponent cText;
            cText.anchor = static_cast<int>(UIAnchor::BottomCenter);
            cText.fontSize = 12.0f;
            cText.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            cText.fontUuid = hudFontUuid;
            cText.layerDepth = hudTextLayerDepth;
            cText.isVisible = false;
            scene->AddComponent<HUDTextComponent>(card.textEntity, cText);

            hudTargetCards.push_back(card);
        }

        // 5. Shot Feedback Banner Text
        hudFeedbackTextEntity = scene->CreateEntity("HUD_Tactical_Feedback_Text");
        HUDTextComponent feedText;
        feedText.anchor = static_cast<int>(UIAnchor::BottomCenter);
        feedText.offset = glm::vec2(-140.0f, -200.0f);
        feedText.fontSize = 15.0f;
        feedText.color = glm::vec4(1.0f, 0.9f, 0.3f, 1.0f);
        feedText.fontUuid = hudFontUuid;
        feedText.layerDepth = hudTextLayerDepth;
        feedText.isVisible = false;
        scene->AddComponent<HUDTextComponent>(hudFeedbackTextEntity, feedText);

        UpdatePawnHUD();
    }

    void TacticalSceneManagerSystem::DestroySelectedPawnHUDEntities() {
        if (!scene) return;

        auto destroyEnt = [&](Entity& e) {
            if (e != MAX_ENTITIES && scene->IsEntityActive(e)) {
                scene->DestroyEntity(e);
            }
            e = MAX_ENTITIES;
        };

        destroyEnt(hudHealthBackgroundEntity);
        destroyEnt(hudHealthFillEntity);
        destroyEnt(hudNameTextEntity);
        destroyEnt(hudStatsTextEntity);
        destroyEnt(hudActionButtonEntity);
        destroyEnt(hudActionTextEntity);
        destroyEnt(hudTargetHeaderEntity);
        destroyEnt(hudFeedbackTextEntity);

        for (auto& card : hudTargetCards) {
            destroyEnt(card.bgEntity);
            destroyEnt(card.fillEntity);
            destroyEnt(card.textEntity);
        }
        hudTargetCards.clear();
    }

    void TacticalSceneManagerSystem::BindHUDEntities(Entity bgEntity, Entity fillEntity, Entity nameEntity) {
        hudHealthBackgroundEntity = bgEntity;
        hudHealthFillEntity = fillEntity;
        hudNameTextEntity = nameEntity;
        UpdatePawnHUD();
    }

    void TacticalSceneManagerSystem::UpdatePawnHUD() {
        if (!scene) return;

        bool hasBg = (hudHealthBackgroundEntity != MAX_ENTITIES && scene->IsEntityActive(hudHealthBackgroundEntity) && scene->HasComponent<HUDElementComponent>(hudHealthBackgroundEntity));
        bool hasFill = (hudHealthFillEntity != MAX_ENTITIES && scene->IsEntityActive(hudHealthFillEntity) && scene->HasComponent<HUDElementComponent>(hudHealthFillEntity));
        bool hasName = (hudNameTextEntity != MAX_ENTITIES && scene->IsEntityActive(hudNameTextEntity) && scene->HasComponent<HUDTextComponent>(hudNameTextEntity));
        bool hasStats = (hudStatsTextEntity != MAX_ENTITIES && scene->IsEntityActive(hudStatsTextEntity) && scene->HasComponent<HUDTextComponent>(hudStatsTextEntity));
        bool hasActionBg = (hudActionButtonEntity != MAX_ENTITIES && scene->IsEntityActive(hudActionButtonEntity) && scene->HasComponent<HUDElementComponent>(hudActionButtonEntity));
        bool hasActionText = (hudActionTextEntity != MAX_ENTITIES && scene->IsEntityActive(hudActionTextEntity) && scene->HasComponent<HUDTextComponent>(hudActionTextEntity));
        bool hasHeader = (hudTargetHeaderEntity != MAX_ENTITIES && scene->IsEntityActive(hudTargetHeaderEntity) && scene->HasComponent<HUDTextComponent>(hudTargetHeaderEntity));
        bool hasFeedback = (hudFeedbackTextEntity != MAX_ENTITIES && scene->IsEntityActive(hudFeedbackTextEntity) && scene->HasComponent<HUDTextComponent>(hudFeedbackTextEntity));

        if (showPawnHUD && HasSelectedEntity() && scene->HasComponent<PawnComponent>(selectedEntity)) {
            const auto& pawn = scene->GetComponent<PawnComponent>(selectedEntity);
            float curHealth = pawn.currentHealth;
            float maxHealth = pawn.maxHealth;
            float healthRatio = maxHealth > 0.0f ? std::clamp(curHealth / maxHealth, 0.0f, 1.0f) : 0.0f;
            PawnAlliance alliance = pawn.orientation;

            // 1. Right Side: Unit Health & Name
            if (hasBg) {
                auto& bgComp = scene->GetComponent<HUDElementComponent>(hudHealthBackgroundEntity);
                bgComp.isVisible = true;

                if (hasFill) {
                    auto& fillComp = scene->GetComponent<HUDElementComponent>(hudHealthFillEntity);
                    fillComp.isVisible = true;
                    fillComp.anchor = bgComp.anchor;
                    fillComp.offset = bgComp.offset;
                    fillComp.size.x = bgComp.size.x * healthRatio;
                    fillComp.size.y = bgComp.size.y;
                    fillComp.color = glm::vec4(1.0f - healthRatio, healthRatio, 0.1f, 1.0f);
                }
            } else if (hasFill) {
                auto& fillComp = scene->GetComponent<HUDElementComponent>(hudHealthFillEntity);
                fillComp.isVisible = true;
            }

            if (hasName) {
                auto& textComp = scene->GetComponent<HUDTextComponent>(hudNameTextEntity);
                textComp.isVisible = true;
                std::string pName = !pawn.name.empty() ? pawn.name : (scene->HasComponent<NameComponent>(selectedEntity) ? scene->GetComponent<NameComponent>(selectedEntity).name : "Pawn");
                textComp.text = pName;
                textComp.color = (alliance == PawnAlliance::Player) ? glm::vec4(0.3f, 0.75f, 1.0f, 1.0f) :
                                 ((alliance == PawnAlliance::Enemy) ? glm::vec4(1.0f, 0.35f, 0.35f, 1.0f) : glm::vec4(1.0f, 0.85f, 0.25f, 1.0f));
            }

            if (hasStats) {
                auto& statsComp = scene->GetComponent<HUDTextComponent>(hudStatsTextEntity);
                statsComp.isVisible = true;
                std::string allianceTag = (alliance == PawnAlliance::Player) ? "[PLAYER]" : ((alliance == PawnAlliance::Enemy) ? "[ENEMY]" : "[NEUT]");
                char statBuf[64];
                snprintf(statBuf, sizeof(statBuf), "%s %.0f/%.0f HP | DMG: %.0f", allianceTag.c_str(), curHealth, maxHealth, pawn.attackDamage);
                statsComp.text = statBuf;
            }

            // 2. Middle: Single Shoot / Fire Button
            if (hasActionBg) {
                auto& actBg = scene->GetComponent<HUDElementComponent>(hudActionButtonEntity);
                actBg.isVisible = true;
                actBg.color = isShootModeActive ? glm::vec4(0.85f, 0.2f, 0.2f, 0.95f) : glm::vec4(0.65f, 0.15f, 0.15f, 0.9f);
            }
            if (hasActionText) {
                auto& actText = scene->GetComponent<HUDTextComponent>(hudActionTextEntity);
                actText.isVisible = true;
                actText.text = isShootModeActive ? "FIRE (Space)" : "SHOOT (1)";
            }

            // 3. Middle: Target Cards & Header (when in Shoot Mode)
            auto targetableUnits = GetTargetableUnits();
            if (isShootModeActive) {
                if (hasHeader) {
                    auto& headComp = scene->GetComponent<HUDTextComponent>(hudTargetHeaderEntity);
                    headComp.isVisible = true;
                    headComp.text = targetableUnits.empty() ? "No targetable units" : "TARGETS (Click or Tab/Arrows to cycle)";
                }

                float cardW = 105.0f;
                float cardH = 42.0f;
                float spacing = 8.0f;
                size_t numUnits = std::min(targetableUnits.size(), hudTargetCards.size());
                float totalW = numUnits > 0 ? (numUnits * cardW + (numUnits - 1) * spacing) : cardW;
                float startX = -totalW * 0.5f;
                float startY = -120.0f;

                for (size_t i = 0; i < hudTargetCards.size(); ++i) {
                    auto& card = hudTargetCards[i];
                    if (i < targetableUnits.size()) {
                        Entity uEnt = targetableUnits[i];
                        card.entityRef = uEnt;
                        card.cachedOffset = glm::vec2(startX + i * (cardW + spacing), startY);
                        card.cachedSize = glm::vec2(cardW, cardH);

                        bool isTargeted = (uEnt == targetUnitEntity);

                        PawnAlliance uAlliance = PawnAlliance::Enemy;
                        std::string uName = "Target";
                        float uCurHp = 100.0f, uMaxHp = 100.0f;
                        if (scene->HasComponent<PawnComponent>(uEnt)) {
                            const auto& pc = scene->GetComponent<PawnComponent>(uEnt);
                            uAlliance = pc.orientation;
                            if (!pc.name.empty()) uName = pc.name;
                            uCurHp = pc.currentHealth;
                            uMaxHp = pc.maxHealth;
                        } else if (scene->HasComponent<NameComponent>(uEnt)) {
                            uName = scene->GetComponent<NameComponent>(uEnt).name;
                        }

                        float uRatio = uMaxHp > 0.0f ? std::clamp(uCurHp / uMaxHp, 0.0f, 1.0f) : 0.0f;

                        if (card.bgEntity != MAX_ENTITIES && scene->IsEntityActive(card.bgEntity) && scene->HasComponent<HUDElementComponent>(card.bgEntity)) {
                            auto& cBg = scene->GetComponent<HUDElementComponent>(card.bgEntity);
                            cBg.isVisible = true;
                            cBg.offset = card.cachedOffset;
                            cBg.size = card.cachedSize;
                            cBg.color = isTargeted ? glm::vec4(0.45f, 0.28f, 0.15f, 0.95f) : glm::vec4(0.12f, 0.15f, 0.2f, 0.9f);
                        }

                        if (card.fillEntity != MAX_ENTITIES && scene->IsEntityActive(card.fillEntity) && scene->HasComponent<HUDElementComponent>(card.fillEntity)) {
                            auto& cFill = scene->GetComponent<HUDElementComponent>(card.fillEntity);
                            cFill.isVisible = true;
                            cFill.offset = card.cachedOffset + glm::vec2(4.0f, cardH - 6.0f);
                            cFill.size = glm::vec2((cardW - 8.0f) * uRatio, 4.0f);
                            cFill.color = (uAlliance == PawnAlliance::Enemy) ? glm::vec4(0.85f, 0.2f, 0.2f, 1.0f) : glm::vec4(0.2f, 0.8f, 0.2f, 1.0f);
                        }

                        if (card.textEntity != MAX_ENTITIES && scene->IsEntityActive(card.textEntity) && scene->HasComponent<HUDTextComponent>(card.textEntity)) {
                            auto& cText = scene->GetComponent<HUDTextComponent>(card.textEntity);
                            cText.isVisible = true;
                            cText.offset = card.cachedOffset + glm::vec2(4.0f, 4.0f);
                            std::string tag = (uAlliance == PawnAlliance::Enemy) ? "[EN]" : ((uAlliance == PawnAlliance::Player) ? "[AL]" : "[NE]");
                            std::string shortName = uName.length() > 6 ? uName.substr(0, 5) + "." : uName;
                            cText.text = tag + " " + shortName + (isTargeted ? " *" : " " + std::to_string((int)uCurHp));
                            cText.color = isTargeted ? glm::vec4(1.0f, 0.85f, 0.2f, 1.0f) :
                                          ((uAlliance == PawnAlliance::Enemy) ? glm::vec4(1.0f, 0.4f, 0.4f, 1.0f) : glm::vec4(0.4f, 0.8f, 1.0f, 1.0f));
                        }
                    } else {
                        card.entityRef = MAX_ENTITIES;
                        if (card.bgEntity != MAX_ENTITIES && scene->IsEntityActive(card.bgEntity) && scene->HasComponent<HUDElementComponent>(card.bgEntity))
                            scene->GetComponent<HUDElementComponent>(card.bgEntity).isVisible = false;
                        if (card.fillEntity != MAX_ENTITIES && scene->IsEntityActive(card.fillEntity) && scene->HasComponent<HUDElementComponent>(card.fillEntity))
                            scene->GetComponent<HUDElementComponent>(card.fillEntity).isVisible = false;
                        if (card.textEntity != MAX_ENTITIES && scene->IsEntityActive(card.textEntity) && scene->HasComponent<HUDTextComponent>(card.textEntity))
                            scene->GetComponent<HUDTextComponent>(card.textEntity).isVisible = false;
                    }
                }
            } else {
                if (hasHeader) {
                    scene->GetComponent<HUDTextComponent>(hudTargetHeaderEntity).isVisible = false;
                }
                for (auto& card : hudTargetCards) {
                    card.entityRef = MAX_ENTITIES;
                    if (card.bgEntity != MAX_ENTITIES && scene->IsEntityActive(card.bgEntity) && scene->HasComponent<HUDElementComponent>(card.bgEntity))
                        scene->GetComponent<HUDElementComponent>(card.bgEntity).isVisible = false;
                    if (card.fillEntity != MAX_ENTITIES && scene->IsEntityActive(card.fillEntity) && scene->HasComponent<HUDElementComponent>(card.fillEntity))
                        scene->GetComponent<HUDElementComponent>(card.fillEntity).isVisible = false;
                    if (card.textEntity != MAX_ENTITIES && scene->IsEntityActive(card.textEntity) && scene->HasComponent<HUDTextComponent>(card.textEntity))
                        scene->GetComponent<HUDTextComponent>(card.textEntity).isVisible = false;
                }
            }

            // 4. Feedback Banner
            if (hasFeedback) {
                auto& feedComp = scene->GetComponent<HUDTextComponent>(hudFeedbackTextEntity);
                if (lastShotFeedbackTimer > 0.0f && !lastShotFeedbackText.empty()) {
                    feedComp.isVisible = true;
                    feedComp.text = lastShotFeedbackText;
                } else {
                    feedComp.isVisible = false;
                }
            }
        } else {
            // Hide all HUD entities when no pawn selected
            if (hasBg) scene->GetComponent<HUDElementComponent>(hudHealthBackgroundEntity).isVisible = false;
            if (hasFill) scene->GetComponent<HUDElementComponent>(hudHealthFillEntity).isVisible = false;
            if (hasName) scene->GetComponent<HUDTextComponent>(hudNameTextEntity).isVisible = false;
            if (hasStats) scene->GetComponent<HUDTextComponent>(hudStatsTextEntity).isVisible = false;
            if (hasActionBg) scene->GetComponent<HUDElementComponent>(hudActionButtonEntity).isVisible = false;
            if (hasActionText) scene->GetComponent<HUDTextComponent>(hudActionTextEntity).isVisible = false;
            if (hasHeader) scene->GetComponent<HUDTextComponent>(hudTargetHeaderEntity).isVisible = false;
            if (hasFeedback) scene->GetComponent<HUDTextComponent>(hudFeedbackTextEntity).isVisible = false;
            for (auto& card : hudTargetCards) {
                card.entityRef = MAX_ENTITIES;
                if (card.bgEntity != MAX_ENTITIES && scene->IsEntityActive(card.bgEntity) && scene->HasComponent<HUDElementComponent>(card.bgEntity))
                    scene->GetComponent<HUDElementComponent>(card.bgEntity).isVisible = false;
                if (card.fillEntity != MAX_ENTITIES && scene->IsEntityActive(card.fillEntity) && scene->HasComponent<HUDElementComponent>(card.fillEntity))
                    scene->GetComponent<HUDElementComponent>(card.fillEntity).isVisible = false;
                if (card.textEntity != MAX_ENTITIES && scene->IsEntityActive(card.textEntity) && scene->HasComponent<HUDTextComponent>(card.textEntity))
                    scene->GetComponent<HUDTextComponent>(card.textEntity).isVisible = false;
            }
        }
    }

    std::optional<CameraComponent> TacticalSceneManagerSystem::GetActiveGameCamera(TransformComponent* outTransform) const {
        if (!scene) return std::nullopt;

        // 1. Search for active camera in scene
        for (auto [cameraEntity, camera, cameraTransform] : scene->GetActiveView<CameraComponent, TransformComponent>()) {
            if (camera.active) {
                if (outTransform) {
                    *outTransform = cameraTransform;
                }
                CameraComponent cam = camera;
                cam.view = glm::inverse(cameraTransform.globalMatrix);
                return cam;
            }
        }

        // 2. Fallback to any camera in scene
        for (auto [cameraEntity, camera, cameraTransform] : scene->GetActiveView<CameraComponent, TransformComponent>()) {
            if (outTransform) {
                *outTransform = cameraTransform;
            }
            CameraComponent cam = camera;
            cam.view = glm::inverse(cameraTransform.globalMatrix);
            return cam;
        }

        return std::nullopt;
    }

    bool TacticalSceneManagerSystem::HandleLeftClick(float screenX, float screenY, float windowWidth, float windowHeight) {
        if (auto hudSys = scene ? scene->GetSystem<HUDSystem>() : nullptr) {
            if (hudSys->HandleClick(screenX, screenY, windowWidth, windowHeight, 0)) {
                return true;
            }
        }
        return SelectAtScreenPos(screenX, screenY, windowWidth, windowHeight);
    }

    bool TacticalSceneManagerSystem::SelectAtScreenPos(float screenX, float screenY, float windowWidth, float windowHeight) {
        if (!scene || windowWidth <= 0.0f || windowHeight <= 0.0f) {
            return false;
        }

        TransformComponent camTransform{};
        auto camOpt = GetActiveGameCamera(&camTransform);
        if (!camOpt.has_value()) {
            return false;
        }

        CameraComponent cam = *camOpt;
        cam.aspectRatio = windowWidth / windowHeight;
        if (cam.fov > 0.0f && cam.nearPlane > 0.0f && cam.farPlane > cam.nearPlane) {
            cam.projection = glm::perspective(glm::radians(cam.fov), cam.aspectRatio, cam.nearPlane, cam.farPlane);
        }

        auto colSys = scene->GetSystem<CollisionSystem>();
        if (!colSys) {
            return false;
        }

        Ray ray = colSys->ScreenToWorldRay(cam, screenX, screenY, windowWidth, windowHeight);

        std::optional<Entity> hitEntity = std::nullopt;
        float closestDist = std::numeric_limits<float>::max();

        // 1. Try standard mesh collision raycast
        auto meshHit = colSys->RayCastClosest(ray);
        if (meshHit.has_value()) {
            hitEntity = meshHit->entity;
            closestDist = meshHit->distance;
        }

        // 2. Raycast directly against PathfindingAgent entities with bounding box
        for (auto [entity, agent, trans] : scene->GetActiveView<PathfindingAgent, TransformComponent>()) {
            float halfSz = static_cast<float>(agent.agentSize) * 0.5f;
            glm::vec3 boxMin(-halfSz, 0.0f, -halfSz);
            glm::vec3 boxMax(halfSz, static_cast<float>(agent.agentSize) * 1.5f, halfSz);
            float dist = 0.0f;
            if (colSys->RayIntersectsAABB(ray, boxMin, boxMax, trans.globalMatrix, dist)) {
                if (dist < closestDist) {
                    closestDist = dist;
                    hitEntity = entity;
                }
            }
        }

        if (hitEntity.has_value()) {
            Entity e = *hitEntity;
            if (!requirePathfindingAgent || scene->HasComponent<PathfindingAgent>(e)) {
                SetSelectedEntity(e);
                return true;
            }
        }

        ClearSelection();
        return false;
    }

    std::optional<glm::vec3> TacticalSceneManagerSystem::RaycastGroundTarget(float screenX, float screenY, float windowWidth, float windowHeight) const {
        if (!scene || windowWidth <= 0.0f || windowHeight <= 0.0f) {
            return std::nullopt;
        }

        TransformComponent camTransform{};
        auto camOpt = GetActiveGameCamera(&camTransform);
        if (!camOpt.has_value()) {
            return std::nullopt;
        }

        CameraComponent cam = *camOpt;
        cam.aspectRatio = windowWidth / windowHeight;
        if (cam.fov > 0.0f && cam.nearPlane > 0.0f && cam.farPlane > cam.nearPlane) {
            cam.projection = glm::perspective(glm::radians(cam.fov), cam.aspectRatio, cam.nearPlane, cam.farPlane);
        }

        auto colSys = scene->GetSystem<CollisionSystem>();
        if (!colSys) {
            return std::nullopt;
        }

        Ray ray = colSys->ScreenToWorldRay(cam, screenX, screenY, windowWidth, windowHeight);

        // 1. Raycast against mesh entities in the scene (excluding selected entity)
        auto meshHit = colSys->RayCastClosest(ray);
        if (meshHit.has_value() && meshHit->entity != selectedEntity) {
            return meshHit->hitPoint;
        }

        // 2. Raycast against GridFragment entities in the scene
        float closestGridDist = std::numeric_limits<float>::max();
        bool foundGrid = false;
        glm::vec3 gridHitPos{0.0f};

        for (auto [gridEnt, gridFrag, trans] : scene->GetActiveView<GridFragment, TransformComponent>()) {
            float halfW = static_cast<float>(gridFrag.width) * 0.5f;
            float halfH = static_cast<float>(gridFrag.height) * 0.5f;
            glm::vec3 boxMin(-halfW, -0.2f, -halfH);
            glm::vec3 boxMax(halfW, 0.2f, halfH);
            float dist = 0.0f;
            if (colSys->RayIntersectsAABB(ray, boxMin, boxMax, trans.globalMatrix, dist)) {
                if (dist < closestGridDist) {
                    closestGridDist = dist;
                    gridHitPos = ray.origin + ray.direction * dist;
                    foundGrid = true;
                }
            }
        }

        if (foundGrid) {
            return gridHitPos;
        }

        // 3. Fallback ground plane intersection at y = groundY
        float groundY = defaultGroundY;
        if (selectedEntity != MAX_ENTITIES && scene->HasComponent<TransformComponent>(selectedEntity)) {
            groundY = scene->GetComponent<TransformComponent>(selectedEntity).position.y;
        }
        if (std::abs(ray.direction.y) > 1e-6f) {
            float t = (groundY - ray.origin.y) / ray.direction.y;
            if (t > 0.0f) {
                return ray.origin + ray.direction * t;
            }
        }

        return std::nullopt;
    }

    bool TacticalSceneManagerSystem::HandleMouseMove(float screenX, float screenY, float windowWidth, float windowHeight) {
        return HoverAtScreenPos(screenX, screenY, windowWidth, windowHeight);
    }

    bool TacticalSceneManagerSystem::HoverAtScreenPos(float screenX, float screenY, float windowWidth, float windowHeight) {
        if (!HasSelectedEntity()) {
            if (auto tacSys = scene ? scene->GetSystem<TacticalGridSystem>() : nullptr) {
                tacSys->ClearHover();
            }
            return false;
        }

        auto hitPosOpt = RaycastGroundTarget(screenX, screenY, windowWidth, windowHeight);
        if (hitPosOpt.has_value()) {
            if (auto tacSys = scene->GetSystem<TacticalGridSystem>()) {
                tacSys->SetHoverPosition(*hitPosOpt);
            }
            return true;
        } else {
            if (auto tacSys = scene->GetSystem<TacticalGridSystem>()) {
                tacSys->ClearHover();
            }
            return false;
        }
    }

    bool TacticalSceneManagerSystem::HandleRightClick(float screenX, float screenY, float windowWidth, float windowHeight) {
        return MoveSelectedToScreenPos(screenX, screenY, windowWidth, windowHeight);
    }

    bool TacticalSceneManagerSystem::MoveSelectedToScreenPos(float screenX, float screenY, float windowWidth, float windowHeight) {
        if (!HasSelectedEntity() || !scene->HasComponent<PathfindingAgent>(selectedEntity)) {
            return false;
        }

        auto targetWorldPos = RaycastGroundTarget(screenX, screenY, windowWidth, windowHeight);
        if (!targetWorldPos.has_value()) {
            return false;
        }

        return MoveSelectedToWorldPos(*targetWorldPos);
    }

    bool TacticalSceneManagerSystem::MoveSelectedToWorldPos(const glm::vec3& targetWorldPos) {
        if (!HasSelectedEntity() || !scene->HasComponent<PathfindingAgent>(selectedEntity)) {
            return false;
        }

        auto pathfindingSystem = scene->GetSystem<PathfindingSystem>();
        if (!pathfindingSystem) {
            return false;
        }

        int agentSize = 1;
        if (scene->HasComponent<PathfindingAgent>(selectedEntity)) {
            agentSize = scene->GetComponent<PathfindingAgent>(selectedEntity).agentSize;
        }

        glm::ivec2 gridCoord = pathfindingSystem->WorldToGrid(targetWorldPos, agentSize);
        glm::vec3 gridPlacement = pathfindingSystem->GridToWorld(gridCoord, targetWorldPos.y, agentSize);
        bool res = pathfindingSystem->RequestPath(selectedEntity, gridPlacement);

        if (auto tacSys = scene->GetSystem<TacticalGridSystem>()) {
            tacSys->ClearHover();
            tacSys->RefreshReachableRange();
        }

        return res;
    }

    void TacticalSceneManagerSystem::Update(float deltaTime) {
        if (!scene) return;

        if (enableTacticalCamera) {
            UpdateCamera(deltaTime);
        }

        if (lastShotFeedbackTimer > 0.0f) {
            lastShotFeedbackTimer = std::max(0.0f, lastShotFeedbackTimer - deltaTime);
        }

        if (selectedEntity != MAX_ENTITIES) {
            if (!scene->IsEntityActive(selectedEntity) || (requirePathfindingAgent && !scene->HasComponent<PathfindingAgent>(selectedEntity))) {
                ClearSelection();
                return;
            }

            if (scene->HasComponent<TransformComponent>(selectedEntity)) {
                auto& trans = scene->GetComponent<TransformComponent>(selectedEntity);
                auto gizmoSystem = scene->GetSystem<GizmoSystem>();
                if (gizmoSystem) {
                    float sz = 1.0f;
                    if (scene->HasComponent<PathfindingAgent>(selectedEntity)) {
                        sz = static_cast<float>(scene->GetComponent<PathfindingAgent>(selectedEntity).agentSize);
                    }

                    // Selection box indicator
                    if (showSelectionGizmo) {
                        gizmoSystem->DrawCube(trans.position, glm::vec3(sz * 1.1f, 0.1f, sz * 1.1f), glm::vec3(0.1f, 0.9f, 0.1f), 0.0f);
                    }

                    // Health bar indicator above entity
                    if (showHealthGizmo && scene->HasComponent<PawnComponent>(selectedEntity)) {
                        const auto& pawn = scene->GetComponent<PawnComponent>(selectedEntity);
                        float pct = pawn.maxHealth > 0.0f ? std::clamp(pawn.currentHealth / pawn.maxHealth, 0.0f, 1.0f) : 0.0f;
                        float barWidth = std::max(0.6f, sz * 0.8f);
                        float barHeight = 0.08f;
                        float barDepth = 0.08f;
                        glm::vec3 barCenter = trans.position + glm::vec3(0.0f, sz * 1.4f + 0.3f, 0.0f);

                        // Background bar (dark background)
                        gizmoSystem->DrawCube(barCenter, glm::vec3(barWidth, barHeight, barDepth), glm::vec3(0.2f, 0.2f, 0.2f), 0.0f);

                        // Foreground bar (health color)
                        if (pct > 0.0f) {
                            float fgWidth = barWidth * pct;
                            float xOffset = (fgWidth - barWidth) * 0.5f;
                            glm::vec3 fgCenter = barCenter + glm::vec3(xOffset, 0.0f, 0.01f);
                            glm::vec3 healthColor = glm::vec3(1.0f - pct, pct, 0.0f);
                            gizmoSystem->DrawCube(fgCenter, glm::vec3(fgWidth, barHeight * 0.9f, barDepth * 1.1f), healthColor, 0.0f);
                        }
                    }

                    // Shoot targeting visualizer line & marker
                    if (isShootModeActive) {
                        Entity currentTarget = (targetUnitEntity != MAX_ENTITIES && scene->IsEntityActive(targetUnitEntity)) ? targetUnitEntity :
                                               ((hoveredUnitEntity != MAX_ENTITIES && scene->IsEntityActive(hoveredUnitEntity)) ? hoveredUnitEntity : Entity(MAX_ENTITIES));
                        if (currentTarget != MAX_ENTITIES && currentTarget != selectedEntity && scene->HasComponent<TransformComponent>(currentTarget)) {
                            const auto& tTrans = scene->GetComponent<TransformComponent>(currentTarget);
                            glm::vec3 startPos = trans.position + glm::vec3(0.0f, 0.8f, 0.0f);
                            glm::vec3 endPos = tTrans.position + glm::vec3(0.0f, 0.8f, 0.0f);

                            glm::vec3 targetColor(1.0f, 0.25f, 0.25f);
                            if (scene->HasComponent<PawnComponent>(currentTarget)) {
                                auto all = scene->GetComponent<PawnComponent>(currentTarget).orientation;
                                if (all == PawnAlliance::Player) targetColor = glm::vec3(0.25f, 0.65f, 1.0f);
                                else if (all == PawnAlliance::Neutral) targetColor = glm::vec3(1.0f, 0.8f, 0.2f);
                            }
                            gizmoSystem->DrawRay(startPos, endPos - startPos, targetColor, 0.0f);
                            gizmoSystem->DrawCube(endPos, glm::vec3(0.6f, 0.6f, 0.6f), targetColor, 0.0f);
                        }
                    }
                }
            }
        }

        UpdatePawnHUD();
    }

    std::vector<Entity> TacticalSceneManagerSystem::GetAllTacticalUnits() const {
        std::vector<Entity> units;
        if (!scene) return units;

        for (auto [e, pawn] : scene->GetActiveView<PawnComponent>()) {
            if (scene->IsEntityActive(e)) {
                units.push_back(e);
            }
        }
        for (auto [e, agent] : scene->GetActiveView<PathfindingAgent>()) {
            if (scene->IsEntityActive(e) && std::find(units.begin(), units.end(), e) == units.end()) {
                units.push_back(e);
            }
        }
        return units;
    }

    std::vector<Entity> TacticalSceneManagerSystem::GetTargetableUnits(std::optional<PawnAlliance> allianceFilter) const {
        std::vector<Entity> allUnits = GetAllTacticalUnits();
        std::vector<Entity> filtered;
        for (Entity e : allUnits) {
            if (e == selectedEntity) continue;
            if (!scene || !scene->IsEntityActive(e)) continue;

            if (allianceFilter.has_value()) {
                if (scene->HasComponent<PawnComponent>(e)) {
                    if (scene->GetComponent<PawnComponent>(e).orientation == allianceFilter.value()) {
                        filtered.push_back(e);
                    }
                }
            } else {
                filtered.push_back(e);
            }
        }
        return filtered;
    }

    void TacticalSceneManagerSystem::SetShootMode(bool active) {
        isShootModeActive = active;
        if (isShootModeActive) {
            if (targetUnitEntity == MAX_ENTITIES || targetUnitEntity == selectedEntity || !scene || !scene->IsEntityActive(targetUnitEntity)) {
                auto enemies = GetTargetableUnits(PawnAlliance::Enemy);
                if (!enemies.empty()) {
                    targetUnitEntity = enemies[0];
                } else {
                    auto all = GetTargetableUnits();
                    if (!all.empty()) {
                        targetUnitEntity = all[0];
                    } else {
                        targetUnitEntity = MAX_ENTITIES;
                    }
                }
            }
        }
        UpdatePawnHUD();
    }

    void TacticalSceneManagerSystem::ToggleShootMode() {
        SetShootMode(!isShootModeActive);
    }

    void TacticalSceneManagerSystem::SelectTargetUnit(Entity target) {
        if (target != MAX_ENTITIES && scene && scene->IsEntityActive(target)) {
            targetUnitEntity = target;
        } else {
            targetUnitEntity = MAX_ENTITIES;
        }
        UpdatePawnHUD();
    }

    void TacticalSceneManagerSystem::ClearTargetUnit() {
        targetUnitEntity = MAX_ENTITIES;
        UpdatePawnHUD();
    }

    void TacticalSceneManagerSystem::CycleNextTarget() {
        auto targets = GetTargetableUnits();
        if (targets.empty()) {
            targetUnitEntity = MAX_ENTITIES;
            UpdatePawnHUD();
            return;
        }
        if (targetUnitEntity == MAX_ENTITIES) {
            targetUnitEntity = targets[0];
            UpdatePawnHUD();
            return;
        }
        auto it = std::find(targets.begin(), targets.end(), targetUnitEntity);
        if (it != targets.end()) {
            size_t idx = std::distance(targets.begin(), it);
            targetUnitEntity = targets[(idx + 1) % targets.size()];
        } else {
            targetUnitEntity = targets[0];
        }
        UpdatePawnHUD();
    }

    void TacticalSceneManagerSystem::CyclePrevTarget() {
        auto targets = GetTargetableUnits();
        if (targets.empty()) {
            targetUnitEntity = MAX_ENTITIES;
            UpdatePawnHUD();
            return;
        }
        if (targetUnitEntity == MAX_ENTITIES) {
            targetUnitEntity = targets.back();
            UpdatePawnHUD();
            return;
        }
        auto it = std::find(targets.begin(), targets.end(), targetUnitEntity);
        if (it != targets.end()) {
            size_t idx = std::distance(targets.begin(), it);
            targetUnitEntity = targets[(idx + targets.size() - 1) % targets.size()];
        } else {
            targetUnitEntity = targets.back();
        }
        UpdatePawnHUD();
    }

    bool TacticalSceneManagerSystem::ExecuteShoot(Entity target) {
        if (!scene) return false;
        Entity targetToShoot = (target != MAX_ENTITIES) ? target : targetUnitEntity;
        if (targetToShoot == MAX_ENTITIES || !scene->IsEntityActive(targetToShoot)) {
            return false;
        }

        float damage = 25.0f;
        if (selectedEntity != MAX_ENTITIES && scene->HasComponent<PawnComponent>(selectedEntity)) {
            damage = scene->GetComponent<PawnComponent>(selectedEntity).attackDamage;
        }

        std::string targetName = "Target";
        if (scene->HasComponent<PawnComponent>(targetToShoot)) {
            auto& tp = scene->GetComponent<PawnComponent>(targetToShoot);
            tp.currentHealth = std::max(0.0f, tp.currentHealth - damage);
            if (!tp.name.empty()) targetName = tp.name;
        } else if (scene->HasComponent<NameComponent>(targetToShoot)) {
            targetName = scene->GetComponent<NameComponent>(targetToShoot).name;
        }

        lastShotFeedbackText = "Shot " + targetName + " for " + std::to_string(static_cast<int>(damage)) + " DMG!";
        lastShotFeedbackTimer = 3.0f;

        if (selectedEntity != MAX_ENTITIES && scene->HasComponent<TransformComponent>(selectedEntity) && scene->HasComponent<TransformComponent>(targetToShoot)) {
            if (auto gizmoSys = scene->GetSystem<GizmoSystem>()) {
                const auto& sTrans = scene->GetComponent<TransformComponent>(selectedEntity);
                const auto& tTrans = scene->GetComponent<TransformComponent>(targetToShoot);
                glm::vec3 startPos = sTrans.position + glm::vec3(0.0f, 0.8f, 0.0f);
                glm::vec3 endPos = tTrans.position + glm::vec3(0.0f, 0.8f, 0.0f);
                gizmoSys->DrawRay(startPos, endPos - startPos, glm::vec3(1.0f, 0.9f, 0.2f), 5);
            }
        }

        UpdatePawnHUD();
        return true;
    }

    void TacticalSceneManagerSystem::OnStart() {
        auto hudFontUuidOpt = scene->engine.assetManagerInterface->getAssetUuid("Geist-Black.font");
        if (hudFontUuidOpt.has_value()) {
            hudFontUuid = hudFontUuidOpt.value();
        }
        CreateSelectedPawnHUDEntities();
        targetCameraTarget = cameraTarget;
        targetCameraDistance = cameraDistance;
        targetCameraYaw = cameraYaw;
    }

    void TacticalSceneManagerSystem::PanCamera(const glm::vec2& movementInput, float deltaTime, bool fast) {
        if (glm::length(movementInput) < 1e-4f) return;
        glm::vec2 normInput = glm::normalize(movementInput);
        float speed = panSpeed * (fast ? fastPanMultiplier : 1.0f);
        float yawRad = glm::radians(cameraYaw);
        glm::vec3 forward = glm::vec3(-std::sin(yawRad), 0.0f, -std::cos(yawRad));
        glm::vec3 right = glm::vec3(std::cos(yawRad), 0.0f, -std::sin(yawRad));

        glm::vec3 delta = (forward * normInput.y + right * normInput.x) * speed * deltaTime;
        targetCameraTarget += delta;
        if (!enableSmoothing) {
            cameraTarget = targetCameraTarget;
        }
    }

    void TacticalSceneManagerSystem::PanCameraByPixels(float deltaX, float deltaY) {
        if (std::abs(deltaX) < 1e-4f && std::abs(deltaY) < 1e-4f) return;
        float yawRad = glm::radians(cameraYaw);
        glm::vec3 forward = glm::vec3(-std::sin(yawRad), 0.0f, -std::cos(yawRad));
        glm::vec3 right = glm::vec3(std::cos(yawRad), 0.0f, -std::sin(yawRad));

        float factor = 0.0015f * cameraDistance;
        glm::vec3 delta = (-right * deltaX + forward * deltaY) * factor;
        targetCameraTarget += delta;
        if (!enableSmoothing) {
            cameraTarget = targetCameraTarget;
        }
    }

    void TacticalSceneManagerSystem::RotateCameraLeft(float stepDegrees) {
        targetCameraYaw -= stepDegrees;
        if (!enableSmoothing) {
            cameraYaw = targetCameraYaw;
        }
    }

    void TacticalSceneManagerSystem::RotateCameraRight(float stepDegrees) {
        targetCameraYaw += stepDegrees;
        if (!enableSmoothing) {
            cameraYaw = targetCameraYaw;
        }
    }

    void TacticalSceneManagerSystem::RotateCameraYaw(float deltaYaw) {
        targetCameraYaw += deltaYaw;
        if (!enableSmoothing) {
            cameraYaw = targetCameraYaw;
        }
    }

    void TacticalSceneManagerSystem::ZoomCamera(float zoomDelta) {
        targetCameraDistance = std::clamp(targetCameraDistance + zoomDelta, minCameraDistance, maxCameraDistance);
        if (!enableSmoothing) {
            cameraDistance = targetCameraDistance;
        }
    }

    void TacticalSceneManagerSystem::ZoomIn(float amount) {
        ZoomCamera(-amount);
    }

    void TacticalSceneManagerSystem::ZoomOut(float amount) {
        ZoomCamera(amount);
    }

    void TacticalSceneManagerSystem::SetCameraPitch(float pitch) {
        cameraPitch = std::clamp(pitch, minPitch, maxPitch);
    }

    void TacticalSceneManagerSystem::FocusOnPosition(const glm::vec3& targetPos, bool immediate) {
        targetCameraTarget = targetPos;
        if (immediate || !enableSmoothing) {
            cameraTarget = targetPos;
        }
    }

    void TacticalSceneManagerSystem::FocusOnEntity(Entity entity, bool immediate) {
        if (!scene || entity == MAX_ENTITIES || !scene->IsEntityActive(entity)) return;
        if (scene->HasComponent<TransformComponent>(entity)) {
            FocusOnPosition(scene->GetComponent<TransformComponent>(entity).position, immediate);
        }
    }

    void TacticalSceneManagerSystem::FocusOnSelected(bool immediate) {
        if (HasSelectedEntity()) {
            FocusOnEntity(selectedEntity, immediate);
        }
    }

    void TacticalSceneManagerSystem::SetCameraTarget(const glm::vec3& target, bool immediate) {
        targetCameraTarget = target;
        if (immediate || !enableSmoothing) {
            cameraTarget = target;
        }
    }

    void TacticalSceneManagerSystem::SetCameraDistance(float distance, bool immediate) {
        targetCameraDistance = std::clamp(distance, minCameraDistance, maxCameraDistance);
        if (immediate || !enableSmoothing) {
            cameraDistance = targetCameraDistance;
        }
    }

    void TacticalSceneManagerSystem::SetCameraYaw(float yaw, bool immediate) {
        targetCameraYaw = yaw;
        if (immediate || !enableSmoothing) {
            cameraYaw = yaw;
        }
    }

    Entity TacticalSceneManagerSystem::GetActiveGameCameraEntity() const {
        if (!scene) return MAX_ENTITIES;
        for (auto [cameraEntity, camera, cameraTransform] : scene->GetActiveView<CameraComponent, TransformComponent>()) {
            if (camera.active) {
                return cameraEntity;
            }
        }
        for (auto [cameraEntity, camera, cameraTransform] : scene->GetActiveView<CameraComponent, TransformComponent>()) {
            return cameraEntity;
        }
        return MAX_ENTITIES;
    }

    void TacticalSceneManagerSystem::ApplyTacticalCameraToActiveCamera() {
        if (!scene) return;
        Entity camEnt = GetActiveGameCameraEntity();
        if (camEnt == MAX_ENTITIES || !scene->HasComponent<TransformComponent>(camEnt)) {
            return;
        }

        auto& trans = scene->GetComponent<TransformComponent>(camEnt);
        float yawRad = glm::radians(cameraYaw);
        float pitchRad = glm::radians(cameraPitch);

        trans.position.x = cameraTarget.x + cameraDistance * std::cos(pitchRad) * std::sin(yawRad);
        trans.position.y = cameraTarget.y + cameraDistance * std::sin(pitchRad);
        trans.position.z = cameraTarget.z + cameraDistance * std::cos(pitchRad) * std::cos(yawRad);

        glm::vec3 lookDir = glm::normalize(cameraTarget - trans.position);
        trans.rotation = glm::quatLookAt(lookDir, glm::vec3(0.0f, 1.0f, 0.0f));
        trans.isDirty = true;
        computeLocalMatrix(trans);
        trans.globalMatrix = trans.localMatrix;

        if (scene->HasComponent<CameraComponent>(camEnt)) {
            auto& cam = scene->GetComponent<CameraComponent>(camEnt);
            updateViewMatrix(cam, trans.globalMatrix);
        }
    }

    void TacticalSceneManagerSystem::UpdateCamera(float deltaTime) {
        if (!enableTacticalCamera || !scene) return;

        Entity camEnt = GetActiveGameCameraEntity();
        if (camEnt != MAX_ENTITIES && scene->HasComponent<TransformComponent>(camEnt)) {
            if (!cameraInitialized) {
                auto& trans = scene->GetComponent<TransformComponent>(camEnt);
                if (glm::length(cameraTarget) == 0.0f && (trans.position.x != 0.0f || trans.position.y != 0.0f || trans.position.z != 0.0f)) {
                    glm::vec3 fwd = getForward(trans);
                    if (std::abs(fwd.y) > 0.01f && trans.position.y > 0.0f) {
                        float distToGround = trans.position.y / (-fwd.y);
                        if (distToGround > 0.0f) {
                            cameraTarget = trans.position + fwd * distToGround;
                            cameraDistance = std::clamp(glm::length(trans.position - cameraTarget), minCameraDistance, maxCameraDistance);
                            targetCameraTarget = cameraTarget;
                            targetCameraDistance = cameraDistance;
                        }
                    }
                }
                cameraInitialized = true;
            }
        }

        if (enableSmoothing && deltaTime > 0.0f) {
            float t = std::clamp(smoothingSpeed * deltaTime, 0.0f, 1.0f);
            cameraTarget = glm::mix(cameraTarget, targetCameraTarget, t);
            cameraDistance = glm::mix(cameraDistance, targetCameraDistance, t);

            float diffYaw = targetCameraYaw - cameraYaw;
            while (diffYaw > 180.0f) diffYaw -= 360.0f;
            while (diffYaw < -180.0f) diffYaw += 360.0f;
            cameraYaw += diffYaw * t;
        } else {
            cameraTarget = targetCameraTarget;
            cameraDistance = targetCameraDistance;
            cameraYaw = targetCameraYaw;
        }

        cameraPitch = std::clamp(cameraPitch, minPitch, maxPitch);
        cameraDistance = std::clamp(cameraDistance, minCameraDistance, maxCameraDistance);

        while (cameraYaw >= 360.0f) { cameraYaw -= 360.0f; targetCameraYaw -= 360.0f; }
        while (cameraYaw < 0.0f) { cameraYaw += 360.0f; targetCameraYaw += 360.0f; }

        ApplyTacticalCameraToActiveCamera();
    }

    bool TacticalSceneManagerSystem::HandleGameWindowInput(float screenX, float screenY, float windowWidth, float windowHeight, float deltaTime, bool isHovered, bool isFocused) {
        if (!enableTacticalCamera || !scene) return false;

#ifdef ENABLE_IMGUI
        ImGuiIO& io = ImGui::GetIO();
        bool fast = io.KeyShift;

        // 1. Keyboard Panning, Rotation, Zoom, Focus, Shoot & Target Navigation
        if ((isHovered || isFocused) && !io.WantTextInput) {
            glm::vec2 moveDir{0.0f};
            if (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_UpArrow)) moveDir.y += 1.0f;
            if (ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_DownArrow)) moveDir.y -= 1.0f;
            if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow)) moveDir.x -= 1.0f;
            if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow)) moveDir.x += 1.0f;

            if (glm::length(moveDir) > 0.0f) {
                PanCamera(moveDir, deltaTime, fast);
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Q)) {
                RotateCameraLeft(rotationStepAngle);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_E)) {
                RotateCameraRight(rotationStepAngle);
            }

            if (ImGui::IsKeyPressed(ImGuiKey_T) || ImGui::IsKeyPressed(ImGuiKey_PageUp)) {
                ZoomIn(zoomSpeed);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_G) || ImGui::IsKeyPressed(ImGuiKey_PageDown)) {
                ZoomOut(zoomSpeed);
            }

            if (ImGui::IsKeyPressed(ImGuiKey_F) || ImGui::IsKeyPressed(ImGuiKey_Home)) {
                FocusOnSelected();
            }

            // Shoot Mode hotkey: '1' or 'S' (when not moving)
            if (HasSelectedEntity()) {
                if (ImGui::IsKeyPressed(ImGuiKey_1)) {
                    ToggleShootMode();
                }

                if (isShootModeActive) {
                    if (ImGui::IsKeyPressed(ImGuiKey_Tab) || ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
                        CycleNextTarget();
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
                        CyclePrevTarget();
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_Space) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                        ExecuteShoot();
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                        SetShootMode(false);
                    }
                }
            }
        }

        // 2. Screen Edge Panning (when mouse is hovered near viewport edge)
        if (enableEdgePanning && isHovered && windowWidth > 0.0f && windowHeight > 0.0f) {
            if (screenX >= 0.0f && screenX <= windowWidth && screenY >= 0.0f && screenY <= windowHeight) {
                glm::vec2 edgeDir{0.0f};
                if (screenX < edgePanMargin) edgeDir.x -= 1.0f;
                if (screenX > windowWidth - edgePanMargin) edgeDir.x += 1.0f;
                if (screenY < edgePanMargin) edgeDir.y += 1.0f;
                if (screenY > windowHeight - edgePanMargin) edgeDir.y -= 1.0f;

                if (glm::length(edgeDir) > 0.0f) {
                    PanCamera(edgeDir, deltaTime, fast);
                }
            }
        }

        // 3. Mouse Wheel Zooming
        if (isHovered && std::abs(io.MouseWheel) > 0.001f) {
            ZoomCamera(-io.MouseWheel * zoomSpeed * (fast ? fastPanMultiplier : 1.0f));
        }

        // 4. Middle Mouse Button Panning Drag
        if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
            isMiddleMouseDragging = true;
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            isMiddleMouseDragging = false;
        }
        if (isMiddleMouseDragging && (std::abs(io.MouseDelta.x) > 0.0f || std::abs(io.MouseDelta.y) > 0.0f)) {
            PanCameraByPixels(io.MouseDelta.x, io.MouseDelta.y);
        }
#endif

        return true;
    }

#ifdef ENABLE_IMGUI
    bool TacticalSceneManagerSystem::CustomDrawSettingsImGui(Scene* scenePtr) {
        bool changed = DrawComponentFields(*this, scenePtr);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (hudHealthBackgroundEntity != MAX_ENTITIES || hudHealthFillEntity != MAX_ENTITIES || hudNameTextEntity != MAX_ENTITIES) {
            ImGui::BulletText("HUD Health Panel: Bound (Right Side)");
            if (ImGui::Button("Destroy HUD Panel Entities")) {
                DestroySelectedPawnHUDEntities();
            }
        } else {
            if (ImGui::Button("Create Selected Pawn HUD Panel (Right Side)")) {
                CreateSelectedPawnHUDEntities();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (HasSelectedEntity()) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "Selected Tactical Agent");
            ImGui::BulletText("Entity ID: %llu", static_cast<unsigned long long>(selectedEntity));

            std::string agentName = GetSelectedEntityName();
            ImGui::BulletText("Name: %s", agentName.empty() ? "(Unnamed)" : agentName.c_str());

            if (scenePtr && scenePtr->HasComponent<PawnComponent>(selectedEntity)) {
                const auto& pawn = scenePtr->GetComponent<PawnComponent>(selectedEntity);
                float pct = pawn.maxHealth > 0.0f ? std::clamp(pawn.currentHealth / pawn.maxHealth, 0.0f, 1.0f) : 0.0f;
                char overlay[64];
                snprintf(overlay, sizeof(overlay), "%.0f / %.0f HP (%.0f%%)", pawn.currentHealth, pawn.maxHealth, pct * 100.0f);
                ImVec4 hpColor = pct > 0.5f ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : (pct > 0.25f ? ImVec4(0.9f, 0.8f, 0.1f, 1.0f) : ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, hpColor);
                ImGui::ProgressBar(pct, ImVec2(-1.0f, 0.0f), overlay);
                ImGui::PopStyleColor();

                const char* allStr = (pawn.orientation == PawnAlliance::Player) ? "Player" :
                                     ((pawn.orientation == PawnAlliance::Enemy) ? "Enemy" : "Neutral");
                ImGui::BulletText("Alliance: %s | DMG: %.0f | Hit: %.0f%%", allStr, pawn.attackDamage, pawn.hitChance);
            } else {
                ImGui::TextDisabled("No PawnComponent attached.");
            }

            if (scenePtr && scenePtr->HasComponent<PathfindingAgent>(selectedEntity)) {
                const auto& agent = scenePtr->GetComponent<PathfindingAgent>(selectedEntity);
                ImGui::BulletText("Size: %d | Speed: %.1f | Allowance: %.1f", agent.agentSize, agent.speed, agent.movementAllowance);
                ImGui::BulletText("Status: %s", agent.isMoving ? "Moving" : (agent.hasTarget ? "Pathing" : "Idle"));
            }

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Tactical Shoot Action");
            if (isShootModeActive) {
                if (ImGui::Button("Cancel Shoot Mode (Esc)")) {
                    SetShootMode(false);
                }
                ImGui::SameLine();
                if (ImGui::Button("Execute Shot (Space)")) {
                    ExecuteShoot();
                }
                if (targetUnitEntity != MAX_ENTITIES) {
                    ImGui::BulletText("Current Target Entity: %llu", static_cast<unsigned long long>(targetUnitEntity));
                }
            } else {
                if (ImGui::Button("Activate Shoot Mode (1)")) {
                    SetShootMode(true);
                }
            }

            ImGui::Spacing();
            if (ImGui::Button("Focus Camera On Selected (F)")) {
                FocusOnSelected();
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear Selection")) {
                ClearSelection();
            }
        } else {
            ImGui::TextDisabled("No agent currently selected.");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "Tactical Camera Quick Actions");
        if (ImGui::Button("Rotate Left (Q)")) {
            RotateCameraLeft(rotationStepAngle);
        }
        ImGui::SameLine();
        if (ImGui::Button("Rotate Right (E)")) {
            RotateCameraRight(rotationStepAngle);
        }
        ImGui::SameLine();
        if (ImGui::Button("Zoom In (T)")) {
            ZoomIn(zoomSpeed);
        }
        ImGui::SameLine();
        if (ImGui::Button("Zoom Out (G)")) {
            ZoomOut(zoomSpeed);
        }

        if (ImGui::Button("Reset Camera Target to Origin")) {
            SetCameraTarget(glm::vec3(0.0f), true);
        }

        return changed;
    }
#endif

} // namespace engine::ecs
