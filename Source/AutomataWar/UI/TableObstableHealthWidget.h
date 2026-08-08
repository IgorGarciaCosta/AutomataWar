#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TableObstableHealthWidget.generated.h"

class UProgressBar;

/** Native data bridge for the Blueprint-authored obstacle health widget. */
UCLASS(Abstract, Blueprintable)
class AUTOMATAWAR_API UTableObstableHealthWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Update the displayed normalized health value. */
    void SetHealthPercent(float InPercent);

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UProgressBar> HealthBar;
};