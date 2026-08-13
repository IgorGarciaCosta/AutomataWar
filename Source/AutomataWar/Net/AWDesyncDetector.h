#pragma once

/**
 * @file AWDesyncDetector.h
 * @brief Client-side deterministic re-simulation and hash comparison for desync detection.
 *
 * After receiving both command lists, seed, and authoritative hash from the server,
 * clients re-simulate locally and compare. Mismatches are logged loudly
 * through LogAutomataNet but do not disconnect (server is authoritative).
 */

#include "CoreMinimal.h"
#include "AutomataWar/Game/AWMatchTypes.h"

/**
 * @brief Utility for client-side desync verification.
 */
struct FAWDesyncDetector
{
    /**
     * @brief Re-simulate a match locally and compare hash to authority.
    * @param Commands0 Commands for slot 0.
    * @param Commands1 Commands for slot 1.
     * @param Seed The simulation seed.
     * @param AuthoritativeHash The server's final hash.
     * @return True if hashes match (no desync).
     */
    static bool VerifyMatch(const TArray<EAWCommand> &Commands0, const TArray<EAWCommand> &Commands1, uint64 Seed, uint64 AuthoritativeHash);
};
