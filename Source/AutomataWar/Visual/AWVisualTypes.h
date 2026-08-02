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
	/** Optional Niagara system: muzzle flash. */
	inline const TCHAR* NS_MuzzleFlash = TEXT("/Game/VFX/NS_MuzzleFlash.NS_MuzzleFlash");
	/** Optional Niagara system: projectile trail. */
	inline const TCHAR* NS_ProjectileTrail = TEXT("/Game/VFX/NS_ProjectileTrail.NS_ProjectileTrail");
	/** Optional Niagara system: impact spark. */
	inline const TCHAR* NS_Impact = TEXT("/Game/VFX/NS_Impact.NS_Impact");
	/** Optional Niagara system: shield bubble. */
	inline const TCHAR* NS_Shield = TEXT("/Game/VFX/NS_Shield.NS_Shield");
	/** Optional Niagara system: destruction explosion. */
	inline const TCHAR* NS_Destruction = TEXT("/Game/VFX/NS_Destruction.NS_Destruction");
	/** Optional SFX: fire. */
	inline const TCHAR* SFX_Fire = TEXT("/Game/Audio/SFX/S_Fire.S_Fire");
	/** Optional SFX: impact. */
	inline const TCHAR* SFX_Impact = TEXT("/Game/Audio/SFX/S_Impact.S_Impact");
	/** Optional SFX: shield activate. */
	inline const TCHAR* SFX_Shield = TEXT("/Game/Audio/SFX/S_Shield.S_Shield");
	/** Optional SFX: move. */
	inline const TCHAR* SFX_Move = TEXT("/Game/Audio/SFX/S_Move.S_Move");
	/** Optional SFX: destruction. */
	inline const TCHAR* SFX_Destroy = TEXT("/Game/Audio/SFX/S_Destroy.S_Destroy");
}

/** Visual config constants. */
namespace AWVisualConfig
{
	/** World-space cell size in Unreal units. */
	inline constexpr float CellSize = 100.f;
	/** Floor Z offset. */
	inline constexpr float FloorZ = 0.f;
	/** Robot visual height offset from floor. */
	inline constexpr float RobotZ = 50.f;
	/** Projectile Z offset. */
	inline constexpr float ProjectileZ = 60.f;
	/** Presentation interpolation speed (units/sec). */
	inline constexpr float InterpSpeed = 8.f;
}
