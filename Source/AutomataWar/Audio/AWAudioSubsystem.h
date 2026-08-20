#pragma once

/**
 * @file AWAudioSubsystem.h
 * @brief Persistent music and ambience mixing for Automata War presentation states.
 */

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AWAudioSubsystem.generated.h"

class UAudioComponent;

/** High-level presentation context requested by UI and gameplay state owners. */
enum class EAWAudioContext : uint8
{
    Frontend,
    Terminal,
    Planning,
    Replay,
    Combat
};

/**
 * Owns non-spatial AW-80 music and ambience for the lifetime of the game instance.
 * Callers report presentation context only; this subsystem owns asset selection,
 * component lifetime, gain staging, and crossfades on the game thread.
 */
UCLASS()
class AUTOMATAWAR_API UAWAudioSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** Stop subsystem-owned audio before the game instance is released. */
    virtual void Deinitialize() override;

    /** Crossfade to the music layer associated with the requested presentation context. */
    void SetContext(EAWAudioContext NewContext);

private:
    /** Lazily create persistent 2D components after a playable world exists. */
    void EnsureComponents();
    /** Create one unity-gain, non-spatial loop without starting playback. */
    UAudioComponent *CreateLoop(const TCHAR *AssetPath) const;
    /** Fade one contextual component in or out without changing its base gain. */
    static void Crossfade(UAudioComponent *Component, float TargetVolume);
    /** Stop and release every audio component owned by this subsystem. */
    void StopAll();

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> TerminalHumComponent;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> FrontendMusicComponent;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> PlanningMusicComponent;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> ReplayMusicComponent;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> CombatMusicComponent;

    EAWAudioContext CurrentContext = EAWAudioContext::Terminal;
    bool bHasContext = false;
};