#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TableObstableHealthWidget.generated.h"

class UProgressBar;

/** Minimal runtime widget used by a TableObstable to display health. */
UCLASS()
class AUTOMATAWAR_API UTableObstableHealthWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Update the displayed normalized health value. */
    void SetHealthPercent(float InPercent);

protected:
    virtual void NativeOnInitialized() override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> HealthBar;

    float HealthPercent = 1.f;
};