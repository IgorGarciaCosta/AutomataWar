#include "AWWidgetBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraEmitter.h"
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

int32 UAWWidgetBlueprintLibrary::SetNiagaraSpriteMaterial(UNiagaraSystem *System, UMaterialInterface *Material)
{
    if (!System || !Material)
        return 0;

    Material->CheckMaterialUsage(MATUSAGE_NiagaraSprites);
    System->Modify();
    int32 ChangedRenderers = 0;
    for (FNiagaraEmitterHandle &EmitterHandle : System->GetEmitterHandles())
    {
        FVersionedNiagaraEmitterData *EmitterData = EmitterHandle.GetEmitterData();
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
            ++ChangedRenderers;
        }
    }

    if (ChangedRenderers > 0)
    {
        System->RequestCompile(true);
        System->MarkPackageDirty();
    }
    return ChangedRenderers;
}