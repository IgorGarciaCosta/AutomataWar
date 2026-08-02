#pragma once

/**
 * @file AWPlayerController.h
 * @brief Player controller handling script submission RPC and local UI API.
 */

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AWMatchTypes.h"
#include "AWPlayerController.generated.h"

/**
 * @brief Player controller that bridges UI input to server-authoritative submission.
 *
 * For online play, SubmitScript sends source via validated Server RPC.
 * For local play, the GameMode calls the same internal handler directly.
 */
UCLASS()
class AUTOMATAWAR_API AAWPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAWPlayerController();

	/** Submit a script to the server for validation and compilation. */
	UFUNCTION(BlueprintCallable, Category = "AutomataWar")
	void SubmitScript(const FString& Source);

	/** Delegate fired when submission result is known (locally). */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSubmissionResult, const FAWValidationResult&, Result);
	UPROPERTY(BlueprintAssignable, Category = "AutomataWar")
	FOnSubmissionResult OnSubmissionResult;

protected:
	/** Server RPC: validated script submission. */
	UFUNCTION(Server, Reliable)
	void Server_SubmitScript(const FString& Source);

	/** Client RPC: notify submission result. */
	UFUNCTION(Client, Reliable)
	void Client_SubmissionResult(bool bSuccess, const FString& ErrorMessage);
};
