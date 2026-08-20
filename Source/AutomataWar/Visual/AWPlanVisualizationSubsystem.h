#pragma once

/**
 * @file AWPlanVisualizationSubsystem.h
 * @brief World-scoped orchestration for cosmetic command-plan previews.
 */

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"
#include "AWPlanVisualizationSubsystem.generated.h"

class AAWArenaRenderer;

/** Rebuilds local-only plan previews from the authoritative round-start state. */
UCLASS()
class AUTOMATAWAR_API UAWPlanVisualizationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    /** Route projection output to the arena's presentation owner. */
    void SetRenderer(AAWArenaRenderer *InRenderer);

    /** Recompute and display one player's current queue. Empty queues clear their projection. */
    bool UpdatePlan(int32 RobotIndex, const TArray<EAWCommand> &Commands,
                    const Automata::SimConfig &Config);

    /** Retain a submitted plan while removing it from view. */
    void HidePlan(int32 RobotIndex);

    /** Rebuild and reveal a retained plan when the player returns to editing. */
    void ShowPlan(int32 RobotIndex);

    void ClearPlan(int32 RobotIndex);
    void ClearAllPlans();

    /** Run one combatant's pending queue without mutating authoritative match state. */
    static bool BuildProjection(int32 RobotIndex, TConstArrayView<EAWCommand> Commands,
                                const Automata::SimConfig &Config,
                                Automata::RobotState &OutInitialRobot,
                                TArray<Automata::StepSnapshot> &OutSnapshots,
                                TArray<Automata::SimEvent> &OutEvents);

private:
    struct FPlanState
    {
        TArray<EAWCommand> Commands;
        Automata::RobotState InitialRobot;
        TArray<Automata::StepSnapshot> Snapshots;
        TArray<Automata::SimEvent> Events;
        bool bHasProjection = false;
        bool bVisible = false;
    };

    void RenderPlan(int32 RobotIndex, int32 AnimatedStep);

    FPlanState Plans[2];
    TWeakObjectPtr<AAWArenaRenderer> Renderer;
};