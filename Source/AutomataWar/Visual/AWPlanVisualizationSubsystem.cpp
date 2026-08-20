/**
 * @file AWPlanVisualizationSubsystem.cpp
 * @brief Deterministic command-plan projection implementation.
 */

#include "AWPlanVisualizationSubsystem.h"
#include "AWArenaRenderer.h"

void UAWPlanVisualizationSubsystem::Deinitialize()
{
    ClearAllPlans();
    Super::Deinitialize();
}

void UAWPlanVisualizationSubsystem::SetRenderer(AAWArenaRenderer *InRenderer)
{
    if (Renderer.Get() == InRenderer)
        return;

    Renderer = InRenderer;
    for (int32 RobotIndex = 0; RobotIndex < 2; ++RobotIndex)
        if (Plans[RobotIndex].bHasProjection)
            RenderPlan(RobotIndex, INDEX_NONE);
}

bool UAWPlanVisualizationSubsystem::UpdatePlan(int32 RobotIndex, const TArray<EAWCommand> &Commands,
                                               const Automata::SimConfig &Config)
{
    if (RobotIndex < 0 || RobotIndex > 1)
        return false;
    if (Commands.IsEmpty())
    {
        ClearPlan(RobotIndex);
        return true;
    }

    const FPlanState &PreviousPlan = Plans[RobotIndex];
    bool bSingleAppend = Commands.Num() == PreviousPlan.Commands.Num() + 1;
    for (int32 Index = 0; bSingleAppend && Index < PreviousPlan.Commands.Num(); ++Index)
        bSingleAppend = PreviousPlan.Commands[Index] == Commands[Index];

    Automata::RobotState InitialRobot;
    TArray<Automata::StepSnapshot> Snapshots;
    TArray<Automata::SimEvent> Events;
    if (!BuildProjection(RobotIndex, Commands, Config, InitialRobot, Snapshots, Events))
        return false;

    int32 AnimatedStep = INDEX_NONE;
    if (bSingleAppend)
    {
        const int32 AddedCommandIndex = Commands.Num() - 1;
        for (const Automata::StepSnapshot &Snapshot : Snapshots)
            if (Snapshot.robots[RobotIndex].currentCommand == AddedCommandIndex)
                AnimatedStep = Snapshot.step;
    }

    FPlanState &Plan = Plans[RobotIndex];
    Plan.Commands = Commands;
    Plan.InitialRobot = InitialRobot;
    Plan.Snapshots = MoveTemp(Snapshots);
    Plan.Events = MoveTemp(Events);
    Plan.bHasProjection = true;
    Plan.bVisible = true;
    RenderPlan(RobotIndex, AnimatedStep);
    return true;
}

void UAWPlanVisualizationSubsystem::HidePlan(int32 RobotIndex)
{
    if (RobotIndex < 0 || RobotIndex > 1 || !Plans[RobotIndex].bHasProjection)
        return;
    Plans[RobotIndex].bVisible = false;
    if (AAWArenaRenderer *ArenaRenderer = Renderer.Get())
        ArenaRenderer->SetPlanProjectionVisible(RobotIndex, false);
}

void UAWPlanVisualizationSubsystem::ShowPlan(int32 RobotIndex)
{
    if (RobotIndex < 0 || RobotIndex > 1 || !Plans[RobotIndex].bHasProjection)
        return;
    Plans[RobotIndex].bVisible = true;
    RenderPlan(RobotIndex, INDEX_NONE);
}

void UAWPlanVisualizationSubsystem::ClearPlan(int32 RobotIndex)
{
    if (RobotIndex < 0 || RobotIndex > 1)
        return;
    Plans[RobotIndex] = FPlanState();
    if (AAWArenaRenderer *ArenaRenderer = Renderer.Get())
        ArenaRenderer->ClearPlanProjection(RobotIndex);
}

void UAWPlanVisualizationSubsystem::ClearAllPlans()
{
    ClearPlan(0);
    ClearPlan(1);
}

void UAWPlanVisualizationSubsystem::RenderPlan(int32 RobotIndex, int32 AnimatedStep)
{
    AAWArenaRenderer *ArenaRenderer = Renderer.Get();
    if (!ArenaRenderer || RobotIndex < 0 || RobotIndex > 1)
        return;

    const FPlanState &Plan = Plans[RobotIndex];
    ArenaRenderer->UpdatePlanProjection(
        RobotIndex, Plan.InitialRobot, Plan.Snapshots, Plan.Events, AnimatedStep);
    if (!Plan.bVisible)
        ArenaRenderer->SetPlanProjectionVisible(RobotIndex, false);
}

bool UAWPlanVisualizationSubsystem::BuildProjection(int32 RobotIndex, TConstArrayView<EAWCommand> Commands,
                                                    const Automata::SimConfig &Config,
                                                    Automata::RobotState &OutInitialRobot,
                                                    TArray<Automata::StepSnapshot> &OutSnapshots,
                                                    TArray<Automata::SimEvent> &OutEvents)
{
    OutInitialRobot = {};
    OutSnapshots.Reset();
    OutEvents.Reset();

    const Automata::RoundState &InitialState = Config.initialState;
    const int64 CellCount = static_cast<int64>(Config.gridWidth) * Config.gridHeight;
    if (RobotIndex < 0 || RobotIndex > 1 || CellCount <= 0 ||
        InitialState.gridWidth != Config.gridWidth || InitialState.gridHeight != Config.gridHeight ||
        InitialState.grid.size() != static_cast<size_t>(CellCount) ||
        InitialState.obstacleHealth.size() != static_cast<size_t>(CellCount) ||
        InitialState.actionPointItemValues.size() != static_cast<size_t>(CellCount))
        return false;

    OutInitialRobot.x = InitialState.robotX[RobotIndex];
    OutInitialRobot.y = InitialState.robotY[RobotIndex];
    OutInitialRobot.facing = InitialState.robotFacing[RobotIndex];
    OutInitialRobot.actionPoints = Config.initialActionPoints[RobotIndex];
    OutInitialRobot.effects = Config.initialEffects[RobotIndex];

    const TArray<EAWCommand> EmptyCommands;
    Automata::Simulation Simulation;
    if (RobotIndex == 0)
        Simulation.RunMatch(Commands, EmptyCommands, Config);
    else
        Simulation.RunMatch(EmptyCommands, Commands, Config);

    OutSnapshots.Reserve(static_cast<int32>(Simulation.GetSnapshots().size()));
    for (const Automata::StepSnapshot &Snapshot : Simulation.GetSnapshots())
        OutSnapshots.Add(Snapshot);
    OutEvents.Reserve(static_cast<int32>(Simulation.GetEvents().size()));
    for (const Automata::SimEvent &Event : Simulation.GetEvents())
        OutEvents.Add(Event);
    return true;
}