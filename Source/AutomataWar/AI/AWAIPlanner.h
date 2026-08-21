#pragma once

/**
 * @file AWAIPlanner.h
 * @brief Stateless deterministic command planning for single-player opponents.
 */

#include "CoreMinimal.h"
#include "AutomataWar/Game/AWMatchTypes.h"

namespace AutomataAI
{
    /**
     * Build a deterministic, non-empty command queue within the supplied AP budget.
     * @param Difficulty Planning depth and access to advanced commands.
     * @param AvailableActionPoints Maximum total queue cost.
     * @param Seed Stable value used to vary left/right routing.
     */
    AUTOMATAWAR_API TArray<EAWCommand> GenerateCommandQueue(
        EAWDifficulty Difficulty, int32 AvailableActionPoints, int32 Seed);
}