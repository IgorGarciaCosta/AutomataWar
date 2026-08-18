#pragma once

/**
 * @file AWWidgetBlueprintLibrary.h
 * @brief Editor scripting bridge for authoring Widget Blueprint trees.
 */

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AWWidgetBlueprintLibrary.generated.h"

class UWidget;
class UWidgetBlueprint;
class UWidgetTree;
class UUserDefinedEnum;

/** Minimal editor bridge for authoring Widget Blueprint trees. */
UCLASS()
class AUTOMATAWAREDITOR_API UAWWidgetBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Editor")
    static UWidgetTree *GetWidgetTree(UWidgetBlueprint *Blueprint);

    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Editor")
    static UWidget *GetRootWidget(UWidgetTree *WidgetTree);

    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Editor")
    static void ClearWidgetTree(UWidgetTree *WidgetTree);

    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Editor")
    static UWidget *ConstructWidget(UWidgetTree *WidgetTree, TSubclassOf<UWidget> WidgetClass, FName WidgetName);

    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Editor")
    static void SetWidgetIsVariable(UWidget *Widget, bool bIsVariable);

    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Editor")
    static void SetRootWidget(UWidgetTree *WidgetTree, UWidget *RootWidget);

    /** Replace a Content Browser enum's entries with the supplied display messages. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Editor")
    static bool SetUserDefinedEnumDisplayNames(UUserDefinedEnum *Enum, const TArray<FString> &DisplayNames);
};