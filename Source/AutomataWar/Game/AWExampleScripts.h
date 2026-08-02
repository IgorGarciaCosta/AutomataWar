#pragma once

/**
 * @file AWExampleScripts.h
 * @brief Three heavily-commented example scripts plus default/training bot.
 *
 * All examples use only supported grammar: MOVE, TURN, SCAN, FIRE, SHIELD, SET, IF, WAIT.
 * There is no STRAFE instruction; Kiter uses TURN+MOVE to reposition laterally.
 */

#include "CoreMinimal.h"

/**
 * @brief Static repository of example scripts accessible from C++.
 *
 * Scripts are returned as FString. Each is a valid program the compiler accepts.
 */
struct FAWExampleScripts
{
    /**
     * Aggressor: Closes distance aggressively and fires constantly.
     * Strategy: Scan for enemy, move toward, fire whenever energy allows.
     * Uses R0 (scan result), R1 (counter) for approach pattern.
     */
    static const FString &Aggressor();

    /**
     * Camper: Holds position, shields when threatened, fires at range.
     * Strategy: Stay put, scan, fire if enemy spotted, shield preemptively.
     * Relies on shield timing to absorb incoming projectiles.
     */
    static const FString &Camper();

    /**
     * Kiter: Maintains distance by repositioning after each attack.
     * Strategy: Scan, fire, then TURN+MOVE away to avoid return fire.
     * No strafe instruction so we turn right, move, turn back to face enemy.
     */
    static const FString &Kiter();

    /**
     * Default/training bot: Simple predictable pattern for testing.
     * Moves forward, fires, waits, repeat. No adaptation.
     */
    static const FString &DefaultBot();
};
