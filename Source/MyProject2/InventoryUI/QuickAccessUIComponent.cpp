#include "QuickAccessUIComponent.h"
#include "InventoryCore/QuickAccessComponent.h"
UQuickAccessUIComponent::UQuickAccessUIComponent(){PrimaryComponentTick.bCanEverTick=false;}
UQuickAccessComponent*UQuickAccessUIComponent::GetQuickAccess()const{return GetOwner()?GetOwner()->FindComponentByClass<UQuickAccessComponent>():nullptr;}
void UQuickAccessUIComponent::ShowRadialMenu(){bRadialMenuVisible=true;}void UQuickAccessUIComponent::HideRadialMenu(){bRadialMenuVisible=false;}
void UQuickAccessUIComponent::SelectByIndex(int32 I){if(UQuickAccessComponent*Q=GetQuickAccess())Q->SelectSlot(I);}
