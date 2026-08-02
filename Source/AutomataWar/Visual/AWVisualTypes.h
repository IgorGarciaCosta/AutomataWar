#pragma once

/**
 * @file AWVisualTypes.h
 * @brief Shared types, log category, and asset path constants for Automata War visuals.
 */

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAutomataVisual, Log, All);

/** Centralized soft-path references for optional visual/audio assets. */
namespace AWVisualAssets
{
    // ─── Materials ───────────────────────────────────────────────────────────
    inline const TCHAR *M_ArenaCell = TEXT("/Game/Art/Materials/M_ArenaCell.M_ArenaCell");
    inline const TCHAR *M_Robot = TEXT("/Game/Art/Materials/M_Robot.M_Robot");
    inline const TCHAR *M_Cover = TEXT("/Game/Art/Materials/M_Cover.M_Cover");
    inline const TCHAR *M_Effect = TEXT("/Game/Art/Materials/M_Effect.M_Effect");

    // ─── Niagara ─────────────────────────────────────────────────────────────
    inline const TCHAR *NS_MuzzleFlash = TEXT("/Game/Art/VFX/NS_MuzzleFlash.NS_MuzzleFlash");
    inline const TCHAR *NS_ProjectileTrail = TEXT("/Game/Art/VFX/NS_ProjectileTrail.NS_ProjectileTrail");
    inline const TCHAR *NS_Impact = TEXT("/Game/Art/VFX/NS_Impact.NS_Impact");
    inline const TCHAR *NS_Shield = TEXT("/Game/Art/VFX/NS_Shield.NS_Shield");
    inline const TCHAR *NS_Destruction = TEXT("/Game/Art/VFX/NS_Destruction.NS_Destruction");

    // ─── SFX ─────────────────────────────────────────────────────────────────
    inline const TCHAR *SFX_Fire = TEXT("/Game/Audio/SFX/S_Fire.S_Fire");
    inline const TCHAR *SFX_Impact = TEXT("/Game/Audio/SFX/S_Impact.S_Impact");
    inline const TCHAR *SFX_Shield = TEXT("/Game/Audio/SFX/S_Shield.S_Shield");
    inline const TCHAR *SFX_Move = TEXT("/Game/Audio/SFX/S_Move.S_Move");
    inline const TCHAR *SFX_Destroy = TEXT("/Game/Audio/SFX/S_Destroy.S_Destroy");
    inline const TCHAR *SFX_MatchStart = TEXT("/Game/Audio/SFX/S_MatchStart.S_MatchStart");
    inline const TCHAR *SFX_MatchEnd = TEXT("/Game/Audio/SFX/S_MatchEnd.S_MatchEnd");

    /** Total count of material asset paths (for test validation). */
    inline constexpr int32 MaterialAssetCount = 4;
    /** Total count of all soft-path assets. */
    inline constexpr int32 TotalAssetPathCount = 16;
}

/** Visual config constants. */
namespace AWVisualConfig
{
    inline constexpr float CellSize = 100.f;
    inline constexpr float GridGap = 2.f;
    inline constexpr float FloorZ = 0.f;
    inline constexpr float RobotZ = 50.f;
    inline constexpr float ProjectileZ = 60.f;
    inline constexpr float InterpSpeed = 8.f;
    /** Duration in seconds for transient VFX components before destruction. */
    inline constexpr float TransientVFXLifespan = 0.3f;
    /** Duration for projectile bolt visuals. */
    inline constexpr float ProjectileBoltLifespan = 0.5f;
    /** Duration for shield bubble visual. */
    inline constexpr float ShieldBubbleLifespan = 2.0f;
}
