#pragma once

#include "AWItem.h"
#include "AWAPItem.generated.h"

/** Presentation actor for one deterministic action-point pickup. */
UCLASS(Blueprintable)
class AUTOMATAWAR_API AAWAPItem : public AAWItem
{
    GENERATED_BODY()

public:
    /** Configure the AP pickup's green coin identity. */
    AAWAPItem();
};