#pragma once

/**
 * @file AWPlayerController.h
 * @brief Player controller handling command submission RPC and local UI API.
 */

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AWMatchTypes.h"
#include "AWPlayerController.generated.h"

/**
 * @brief Player controller that bridges UI input to server-authoritative submission.
 *
 * For online play, SubmitCommands sends actions via a validated Server RPC.
 * For local play, the GameMode calls the same internal handler directly.
 */
UCLASS(Blueprintable)
class AUTOMATAWAR_API AAWPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    /** Configure UI-capable local controller behavior and replication defaults. */
    AAWPlayerController();

    /** Create the local HUD and select the placed presentation camera after world startup. */
    virtual void BeginPlay() override;

    /** Submit one local slot or the owning network player's commands through the authoritative path. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar")
    void SubmitCommands(int32 LocalSlot, const TArray<EAWCommand> &Commands);

    /** Withdraw one local slot or the owning network player's submitted commands. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar")
    void WithdrawCommands(int32 LocalSlot);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCommandRequestResult, int32, Slot, const FAWValidationResult &, Result);
    UPROPERTY(BlueprintAssignable, Category = "AutomataWar")
    FOnCommandRequestResult OnSubmissionResult;

    UPROPERTY(BlueprintAssignable, Category = "AutomataWar")
    FOnCommandRequestResult OnWithdrawalResult;

protected:
    UFUNCTION(Server, Reliable)
    void Server_SubmitCommands(const TArray<EAWCommand> &Commands);

    UFUNCTION(Server, Reliable)
    void Server_WithdrawCommands();

    UFUNCTION(Client, Reliable)
    void Client_SubmissionResult(int32 Slot, bool bSuccess, const FString &ErrorMessage);

    UFUNCTION(Client, Reliable)
    void Client_WithdrawalResult(int32 Slot, bool bSuccess, const FString &ErrorMessage);

    /** Resolve an explicit hot-seat slot or the replicated slot owned by this network player. */
    int32 ResolveCommandSlot(int32 LocalSlot) const;

    /** HUD Blueprint created for the local player. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutomataWar|UI")
    TSubclassOf<class UAWHUDWidget> HUDWidgetClass;

    /** Software cursor Blueprint used throughout the game viewport. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutomataWar|UI")
    TSubclassOf<class UUserWidget> CursorWidgetClass;

    UPROPERTY()
    TObjectPtr<class UAWHUDWidget> HUDWidget;

    UPROPERTY()
    TObjectPtr<class UUserWidget> CursorWidget;
};
