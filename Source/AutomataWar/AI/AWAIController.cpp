#include "AWAIController.h"

AAWAIController::AAWAIController()
{
    PrimaryActorTick.bCanEverTick = false;
    bWantsPlayerState = false;
}

TArray<EAWCommand> AAWAIController::GenerateCommandQueue(EAWDifficulty Difficulty, int32 AvailableActionPoints, int32 Seed)
{
    const EAWCommand Turn = (Seed & 1) == 0 ? EAWCommand::TurnLeft : EAWCommand::TurnRight;
    TArray<EAWCommand> Plan;
    switch (Difficulty)
    {
    case EAWDifficulty::Easy:
        Plan = {EAWCommand::Move, EAWCommand::Wait, Turn, EAWCommand::Fire};
        break;
    case EAWDifficulty::Normal:
        Plan = {EAWCommand::ChargeShield, EAWCommand::Move, EAWCommand::Move,
                Turn, EAWCommand::Fire, EAWCommand::Wait};
        break;
    case EAWDifficulty::Hard:
        Plan = {EAWCommand::ChargeShield, EAWCommand::Accelerate, EAWCommand::Move,
                Turn, EAWCommand::Move, EAWCommand::Fire, EAWCommand::Wait};
        break;
    }

    TArray<EAWCommand> Queue;
    int32 RemainingActionPoints = FMath::Max(0, AvailableActionPoints);
    for (EAWCommand Command : Plan)
    {
        const int32 Cost = GetActionPointCost(Command);
        if (Cost <= RemainingActionPoints)
        {
            Queue.Add(Command);
            RemainingActionPoints -= Cost;
        }
    }

    if (Queue.IsEmpty())
        Queue.Add(EAWCommand::Wait);
    return Queue;
}