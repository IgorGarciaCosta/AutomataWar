#include "TableObstableHealthWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"

void UTableObstableHealthWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
    WidgetTree->RootWidget = HealthBar;

    FProgressBarStyle Style = HealthBar->GetWidgetStyle();
    Style.BackgroundImage.TintColor = FSlateColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.9f));
    Style.FillImage.TintColor = FSlateColor(FLinearColor::White);
    HealthBar->SetWidgetStyle(Style);
    SetHealthPercent(HealthPercent);
}

void UTableObstableHealthWidget::SetHealthPercent(float InPercent)
{
    HealthPercent = FMath::Clamp(InPercent, 0.f, 1.f);
    if (!HealthBar)
        return;

    HealthBar->SetPercent(HealthPercent);
    HealthBar->SetFillColorAndOpacity(FLinearColor::LerpUsingHSV(
        FLinearColor(0.9f, 0.08f, 0.04f), FLinearColor(0.05f, 0.85f, 0.2f), HealthPercent));
}