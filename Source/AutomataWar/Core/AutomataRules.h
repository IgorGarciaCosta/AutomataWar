#pragma once

/**
 * @file AutomataRules.h
 * @brief Balance and compatibility constants for finite command-list matches.
 */

#include <array>
#include <cstdint>

namespace Automata
{

    inline constexpr int32_t DefaultGridWidth = 16;
    inline constexpr int32_t DefaultGridHeight = 16;
    inline constexpr int32_t MaxHP = 100;
    inline constexpr int32_t MaxCommands = 256;
    inline constexpr int32_t ProjectileDamage = 20;
    inline constexpr int32_t ObstacleMaxHealth = 60;
    inline constexpr int32_t InitialActionPoints = 100;
    inline constexpr int32_t MoveActionPointCost = 10;
    inline constexpr int32_t FireActionPointCost = 20;
    inline constexpr int32_t TurnActionPointCost = 5;
    inline constexpr int32_t WaitActionPointCost = 0;
    inline constexpr int32_t ChargeShieldActionPointCost = 20;
    inline constexpr int32_t AccelerateActionPointCost = 30;
    inline constexpr int32_t ActionPointItemCount = 12;
    inline constexpr int32_t ActionPointItemMinValue = 10;
    inline constexpr int32_t ActionPointItemMaxValue = 20;
    inline constexpr int32_t ExtraAmmoItemCount = 2;
    inline constexpr int32_t ShieldItemCount = 2;
    inline constexpr int32_t AcceleratorItemCount = 2;
    inline constexpr int32_t ExtraAmmoDamageBonus = 10;
    inline constexpr int32_t PowerUpDurationRounds = 2;
    inline constexpr int32_t TurnModelVersion = 4;

    /** Cardinal facing values ordered clockwise. */
    enum class Dir : uint8_t
    {
        North,
        East,
        South,
        West
    };

    inline constexpr std::array<int32_t, 4> DirDX = {0, 1, 0, -1};
    inline constexpr std::array<int32_t, 4> DirDY = {-1, 0, 1, 0};

    /** Compact replay format storing one byte per command. */
    inline constexpr uint16_t ReplayVersion = 8;

    /** Changes whenever command semantics or balance changes. */
    inline constexpr uint64_t RulesetHash = []() constexpr -> uint64_t
    {
        uint64_t Hash = 14695981039346656037ULL;
        auto Mix = [&](int64_t Value)
        {
            for (int32_t Byte = 0; Byte < 8; ++Byte)
            {
                Hash ^= static_cast<uint8_t>(Value >> (Byte * 8));
                Hash *= 1099511628211ULL;
            }
        };
        Mix(DefaultGridWidth);
        Mix(DefaultGridHeight);
        Mix(MaxHP);
        Mix(MaxCommands);
        Mix(ProjectileDamage);
        Mix(ObstacleMaxHealth);
        Mix(InitialActionPoints);
        Mix(MoveActionPointCost);
        Mix(FireActionPointCost);
        Mix(TurnActionPointCost);
        Mix(WaitActionPointCost);
        Mix(ChargeShieldActionPointCost);
        Mix(AccelerateActionPointCost);
        Mix(ActionPointItemCount);
        Mix(ActionPointItemMinValue);
        Mix(ActionPointItemMaxValue);
        Mix(ExtraAmmoItemCount);
        Mix(ShieldItemCount);
        Mix(AcceleratorItemCount);
        Mix(ExtraAmmoDamageBonus);
        Mix(PowerUpDurationRounds);
        Mix(TurnModelVersion);
        return Hash;
    }();

} // namespace Automata