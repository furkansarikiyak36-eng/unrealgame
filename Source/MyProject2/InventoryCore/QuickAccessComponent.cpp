#include "QuickAccessComponent.h"
#include "InventoryComponent.h"
UQuickAccessComponent::UQuickAccessComponent(){PrimaryComponentTick.bCanEverTick=false;}
void UQuickAccessComponent::BeginPlay(){Super::BeginPlay();QuickSlots.SetNum(FMath::Max(1,SlotCount));}
bool UQuickAccessComponent::AssignToSlot(FName ID,int32 I){if(I<0||I>=SlotCount||ID.IsNone())return false;if(QuickSlots.Num()<SlotCount)QuickSlots.SetNum(SlotCount);QuickSlots[I]=ID;return true;}
bool UQuickAccessComponent::ClearSlot(int32 I){if(!QuickSlots.IsValidIndex(I))return false;QuickSlots[I]=NAME_None;return true;}
FName UQuickAccessComponent::GetSlotItem(int32 I)const{return QuickSlots.IsValidIndex(I)?QuickSlots[I]:NAME_None;}
bool UQuickAccessComponent::SelectSlot(int32 I){if(I<0||I>=QuickSlots.Num())return false;ActiveSlot=I;if(OnItemEquipped.IsBound())OnItemEquipped.Broadcast(QuickSlots[I],I);return true;}
bool UQuickAccessComponent::CycleNext(){if(QuickSlots.Num()==0)return false;return SelectSlot((ActiveSlot+1+QuickSlots.Num())%QuickSlots.Num());}
bool UQuickAccessComponent::CyclePrevious(){if(QuickSlots.Num()==0)return false;return SelectSlot((ActiveSlot-1+QuickSlots.Num())%QuickSlots.Num());}
bool UQuickAccessComponent::RestoreQuickSlots(const TArray<FName>& Items,int32 InActive){QuickSlots.SetNum(FMath::Max(1,SlotCount));for(int32 i=0;i<FMath::Min(Items.Num(),QuickSlots.Num());++i)QuickSlots[i]=Items[i];ActiveSlot=INDEX_NONE;return InActive<0?true:SelectSlot(InActive);}
