#include "AWWidgetBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Engine/UserDefinedEnum.h"
#include "Kismet2/EnumEditorUtils.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraSystem.h"
#include "WidgetBlueprint.h"

UWidgetTree *UAWWidgetBlueprintLibrary::GetWidgetTree(UWidgetBlueprint *Blueprint)
{
    return Blueprint ? Blueprint->WidgetTree : nullptr;
}

UWidget *UAWWidgetBlueprintLibrary::GetRootWidget(UWidgetTree *WidgetTree)
{
    return WidgetTree ? WidgetTree->RootWidget : nullptr;
}

void UAWWidgetBlueprintLibrary::ClearWidgetTree(UWidgetTree *WidgetTree)
{
    if (!WidgetTree)
        return;

    WidgetTree->Modify();
    if (WidgetTree->RootWidget)
        WidgetTree->RemoveWidget(WidgetTree->RootWidget);
    WidgetTree->RootWidget = nullptr;
}

UWidget *UAWWidgetBlueprintLibrary::ConstructWidget(UWidgetTree *WidgetTree, TSubclassOf<UWidget> WidgetClass, FName WidgetName)
{
    if (!WidgetTree || !WidgetClass)
        return nullptr;

    if (WidgetClass->IsChildOf(UUserWidget::StaticClass()))
    {
        TSubclassOf<UUserWidget> UserWidgetClass = WidgetClass.Get();
        return WidgetTree->ConstructWidget<UUserWidget>(UserWidgetClass, WidgetName);
    }
    return WidgetTree->ConstructWidget<UWidget>(WidgetClass, WidgetName);
}

void UAWWidgetBlueprintLibrary::SetWidgetIsVariable(UWidget *Widget, bool bIsVariable)
{
    if (Widget)
        Widget->bIsVariable = bIsVariable;
}

void UAWWidgetBlueprintLibrary::SetRootWidget(UWidgetTree *WidgetTree, UWidget *RootWidget)
{
    if (!WidgetTree || !RootWidget)
        return;

    WidgetTree->Modify();
    WidgetTree->RootWidget = RootWidget;
}

bool UAWWidgetBlueprintLibrary::SetUserDefinedEnumDisplayNames(UUserDefinedEnum *Enum, const TArray<FString> &DisplayNames)
{
    if (!Enum || DisplayNames.IsEmpty())
        return false;

    while (Enum->NumEnums() > 1)
        FEnumEditorUtils::RemoveEnumeratorFromUserDefinedEnum(Enum, 0);

    for (const FString &DisplayName : DisplayNames)
        FEnumEditorUtils::AddNewEnumeratorForUserDefinedEnum(Enum);
    for (int32 Index = 0; Index < DisplayNames.Num(); ++Index)
        if (!FEnumEditorUtils::SetEnumeratorDisplayName(Enum, Index, FText::FromString(DisplayNames[Index])))
            return false;
    return true;
}

bool UAWWidgetBlueprintLibrary::ConfigureOneShotNiagaraSprites(
    UNiagaraSystem *System, UMaterialInterface *Material)
{
    if (!System || !Material || System->IsLooping())
        return false;

    System->Modify();
    int32 UpdatedRenderers = 0;
    for (FNiagaraEmitterHandle &Handle : System->GetEmitterHandles())
    {
        FVersionedNiagaraEmitter Instance = Handle.GetInstance();
        FVersionedNiagaraEmitterData *EmitterData = Instance.GetEmitterData();
        if (!EmitterData)
            continue;

        for (UNiagaraRendererProperties *Renderer : EmitterData->GetRenderers())
        {
            UNiagaraSpriteRendererProperties *SpriteRenderer = Cast<UNiagaraSpriteRendererProperties>(Renderer);
            if (!SpriteRenderer)
                continue;

            SpriteRenderer->Modify();
            SpriteRenderer->Material = Material;
            SpriteRenderer->PostEditChange();
            ++UpdatedRenderers;
        }
    }

    if (UpdatedRenderers == 0)
        return false;

    System->RequestCompile(true);
    System->PollForCompilationComplete();
    System->PostEditChange();
    return !System->IsLooping();
}