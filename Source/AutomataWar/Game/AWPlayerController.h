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

    /** Submit commands through the authoritative server RPC path. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar")
    void SubmitCommands(const TArray<EAWCommand> &Commands);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSubmissionResult, const FAWValidationResult &, Result);
    UPROPERTY(BlueprintAssignable, Category = "AutomataWar")
    FOnSubmissionResult OnSubmissionResult;

protected:
    UFUNCTION(Server, Reliable)
    void Server_SubmitCommands(const TArray<EAWCommand> &Commands);

    UFUNCTION(Client, Reliable)
    void Client_SubmissionResult(bool bSuccess, const FString &ErrorMessage);

    /** HUD Blueprint created for the local player. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutomataWar|UI")
    TSubclassOf<class UAWHUDWidget> HUDWidgetClass;

    UPROPERTY()
    TObjectPtr<class UAWHUDWidget> HUDWidget;
};
