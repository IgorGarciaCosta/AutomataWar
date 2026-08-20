#include "AWAudioSubsystem.h"
#include "AutomataWar/UI/AWUITypes.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
    const TCHAR *TerminalHumPath = TEXT("/Game/Audio/Ambience/S_AWTerminalHum.S_AWTerminalHum");
    const TCHAR *FrontendMusicPath = TEXT("/Game/Audio/Ambience/S_AWFrontend.S_AWFrontend");
    const TCHAR *PlanningMusicPath = TEXT("/Game/Audio/Ambience/S_AWPlanning.S_AWPlanning");
    const TCHAR *ReplayMusicPath = TEXT("/Game/Audio/Ambience/S_AWReplay.S_AWReplay");
    const TCHAR *CombatMusicPath = TEXT("/Game/Audio/Ambience/S_AWCombat.S_AWCombat");
    const TCHAR *ContextNames[] = {TEXT("Frontend"), TEXT("Terminal"), TEXT("Planning"), TEXT("Replay"), TEXT("Combat")};
}

void UAWAudioSubsystem::Deinitialize()
{
    StopAll();
    Super::Deinitialize();
}

void UAWAudioSubsystem::SetContext(EAWAudioContext NewContext)
{
    if (bHasContext && CurrentContext == NewContext)
        return;

    EnsureComponents();
    if (TerminalHumComponent && !TerminalHumComponent->IsPlaying())
        TerminalHumComponent->FadeIn(1.5f, 0.10f, 0.f, EAudioFaderCurve::Logarithmic);

    Crossfade(FrontendMusicComponent, NewContext == EAWAudioContext::Frontend ? 0.28f : 0.f);
    Crossfade(PlanningMusicComponent, NewContext == EAWAudioContext::Planning ? 0.24f : 0.f);
    Crossfade(ReplayMusicComponent, NewContext == EAWAudioContext::Replay ? 0.20f : 0.f);
    Crossfade(CombatMusicComponent, NewContext == EAWAudioContext::Combat ? 0.28f : 0.f);

    CurrentContext = NewContext;
    bHasContext = true;
    UE_LOG(LogAutomataUI, Log, TEXT("Audio context: %s"), ContextNames[static_cast<uint8>(NewContext)]);
}

void UAWAudioSubsystem::EnsureComponents()
{
    if (!TerminalHumComponent)
        TerminalHumComponent = CreateLoop(TerminalHumPath);
    if (!FrontendMusicComponent)
        FrontendMusicComponent = CreateLoop(FrontendMusicPath);
    if (!PlanningMusicComponent)
        PlanningMusicComponent = CreateLoop(PlanningMusicPath);
    if (!ReplayMusicComponent)
        ReplayMusicComponent = CreateLoop(ReplayMusicPath);
    if (!CombatMusicComponent)
        CombatMusicComponent = CreateLoop(CombatMusicPath);
}

UAudioComponent *UAWAudioSubsystem::CreateLoop(const TCHAR *AssetPath) const
{
    USoundBase *Sound = LoadObject<USoundBase>(nullptr, AssetPath);
    if (!Sound)
    {
        UE_LOG(LogAutomataUI, Warning, TEXT("Music asset is missing: %s"), AssetPath);
        return nullptr;
    }

    return UGameplayStatics::CreateSound2D(this, Sound, 1.f, 1.f, 0.f, nullptr, true, false);
}

void UAWAudioSubsystem::Crossfade(UAudioComponent *Component, float TargetVolume)
{
    if (!Component)
        return;

    if (TargetVolume > 0.f)
    {
        if (Component->IsPlaying())
            Component->AdjustVolume(0.8f, TargetVolume, EAudioFaderCurve::Logarithmic);
        else
            Component->FadeIn(0.8f, TargetVolume, 0.f, EAudioFaderCurve::Logarithmic);
    }
    else if (Component->IsPlaying())
    {
        Component->FadeOut(0.8f, 0.f, EAudioFaderCurve::Logarithmic);
    }
}

void UAWAudioSubsystem::StopAll()
{
    UAudioComponent *Components[] = {
        TerminalHumComponent, FrontendMusicComponent, PlanningMusicComponent, ReplayMusicComponent, CombatMusicComponent};
    for (UAudioComponent *Component : Components)
    {
        if (Component)
            Component->Stop();
    }

    TerminalHumComponent = nullptr;
    FrontendMusicComponent = nullptr;
    PlanningMusicComponent = nullptr;
    ReplayMusicComponent = nullptr;
    CombatMusicComponent = nullptr;
    bHasContext = false;
}