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
class UMaterialInterface;
class UNiagaraSystem;

/** Minimal generic bridge used by BuildScripts/BuildHUD.py. */
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

    /** Replace every sprite renderer material in a Niagara system and return the number changed. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Editor")
    static int32 SetNiagaraSpriteMaterial(UNiagaraSystem *System, UMaterialInterface *Material);
};