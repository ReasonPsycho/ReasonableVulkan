#ifndef REASONABLEVULKAN_PAWNCOMPONENT_HPP
#define REASONABLEVULKAN_PAWNCOMPONENT_HPP

#include <string>
#include <utility>
#include "ecs/Component.hpp"

namespace engine::ecs
{
    enum class PawnAlliance
    {
        Player = 0,
        Enemy = 1,
        Neutral = 2
    };

    struct PawnComponent : public Component
    {
        [[=Tooltip{"Name of the pawn"}]]
        std::string name{"Pawn"};

        [[=Range{0.0f, 10000.0f, 1.0f}, =Tooltip{"Current health of the pawn"}]]
        float currentHealth{100.0f};

        [[=Range{1.0f, 10000.0f, 1.0f}, =Tooltip{"Maximum health of the pawn"}]]
        float maxHealth{100.0f};

        [[=Tooltip{"Alliance of the pawn"}]]
        PawnAlliance orientation{PawnAlliance::Player};

        [[=Range{0.0f, 1000.0f, 1.0f}, =Tooltip{"Base attack damage for shooting"}]]
        float attackDamage{25.0f};

        [[=Range{0.0f, 100.0f, 1.0f}, =Tooltip{"Hit chance percentage (0-100)"}]]
        float hitChance{75.0f};

        [[=Range{1.0f, 100.0f, 1.0f}, =Tooltip{"Attack range in world units"}]]
        float attackRange{20.0f};

        PawnComponent() = default;
        PawnComponent(std::string pawnName, float current, float max, PawnAlliance alliance = PawnAlliance::Player)
            : name(std::move(pawnName)), currentHealth(current), maxHealth(max), orientation(alliance) {}
        PawnComponent(float current, float max, PawnAlliance alliance = PawnAlliance::Player)
            : name("Pawn"), currentHealth(current), maxHealth(max), orientation(alliance) {}
        explicit PawnComponent(float max, PawnAlliance alliance = PawnAlliance::Player)
            : name("Pawn"), currentHealth(max), maxHealth(max), orientation(alliance) {}
    };
}

#endif //REASONABLEVULKAN_PAWNCOMPONENT_HPP
