#include "AWAPItem.h"
#include "AWVisualTypes.h"

AAWAPItem::AAWAPItem()
{
    ItemColor = FLinearColor(0.35f, 1.f, 0.28f);
    PickupEffectPath = AWVisualAssets::NS_ActionPointPickup;
    PickupSoundPitch = 1.35f;
}