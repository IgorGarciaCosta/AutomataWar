#include "TableObstableHealthWidget.h"

#include "Components/ProgressBar.h"

void UTableObstableHealthWidget::SetHealthPercent(float InPercent)
{
    if (HealthBar)
        HealthBar->SetPercent(FMath::Clamp(InPercent, 0.f, 1.f));
}