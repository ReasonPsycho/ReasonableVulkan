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
        Enemy = 1
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

        PawnComponent() = default;
        PawnComponent(std::string pawnName, float current, float max)
            : name(std::move(pawnName)), currentHealth(current), maxHealth(max) {}
        PawnComponent(float current, float max)
            : name("Pawn"), currentHealth(current), maxHealth(max) {}
        explicit PawnComponent(float max)
            : name("Pawn"), currentHealth(max), maxHealth(max) {}
    };
}

#endif //REASONABLEVULKAN_PAWNCOMPONENT_HPP
