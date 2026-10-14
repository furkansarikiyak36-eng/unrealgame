#include "InventoryComponent.h"
#include "InventoryItem.h"

UInventoryComponent::UInventoryComponent() { PrimaryComponentTick.bCanEverTick=false; }
void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    MaxSlots=FMath::Max(1,MaxSlots); Slots.SetNum(MaxSlots);
    for (int32 i=0;i<Slots.Num();++i) Slots[i].SlotIndex=i;
    for (UInventoryItemData* D : ItemDefinitions) UInventoryItemLibrary::RegisterItemData(D);
}
UInventoryItemData* UInventoryComponent::GetItemData(FName ID) const
{
    for (UInventoryItemData* D : ItemDefinitions) if (D && D->ItemID==ID) return D;
    return UInventoryItemLibrary::FindItemData(ID);
}
int32 UInventoryComponent::FindStackSlot(FName ID) const
{
    const int32 ActiveSlotCount = FMath::Clamp(MaxSlots, 0, Slots.Num());
    for (int32 i=0; i<ActiveSlotCount; ++i)
        if (!Slots[i].bLocked && Slots[i].IsOccupied() && Slots[i].Item.ItemID==ID && !Slots[i].Item.IsFull()) return i;
    return INDEX_NONE;
}
int32 UInventoryComponent::FindEmptySlot() const
{
    for (int32 i=0;i<FMath::Min(MaxSlots,Slots.Num());++i) if (!Slots[i].bLocked && !Slots[i].IsOccupied()) return i;
    return INDEX_NONE;
}
bool UInventoryComponent::CanFitItem(const FInventoryItem& Item, int32 Count) const
{
    if (Count<=0 || Item.ItemID.IsNone()) return false;
    if (!bIgnoreWeight && GetTotalWeight()+Item.Weight*Count > MaxWeight+KINDA_SMALL_NUMBER) return false;
    // Capacity checks and mutation must use the same addressable slot range.
    // MaxSlots may change after BeginPlay while Slots retains its old allocation.
    const int32 ActiveSlotCount = FMath::Clamp(MaxSlots, 0, Slots.Num());
    int32 Space=0, NeededSlots=0;
    if (Item.MaxStack>1)
        for (int32 i=0; i<ActiveSlotCount; ++i)
        {
            const FInventorySlot& S=Slots[i];
            if (!S.bLocked && S.IsOccupied() && S.Item.ItemID==Item.ItemID)
                Space+=FMath::Max(0,S.Item.MaxStack-S.Item.Count);
        }
    const int32 Remaining=FMath::Max(0,Count-Space);
    NeededSlots = Item.MaxStack>1 ? FMath::DivideAndRoundUp(Remaining,FMath::Max(1,Item.MaxStack)) : Remaining;
    int32 Free=0;
    for (int32 i=0; i<ActiveSlotCount; ++i)
        if (!Slots[i].bLocked && !Slots[i].IsOccupied()) ++Free;
    return NeededSlots <= Free;
}
bool UInventoryComponent::CanAddItem(FName ID, int32 Count) const
{
    UInventoryItemData* D=GetItemData(ID); if (!D) return false;
    return CanFitItem(D->ToInventoryItem(1), Count);
}
EInventoryResult UInventoryComponent::AddItem(FName ID, int32 Count)
{
    if (Count<=0 || ID.IsNone()) return EInventoryResult::InvalidRequest;
    UInventoryItemData* Data=GetItemData(ID); if (!Data) return EInventoryResult::NoRecipeMatch;
    const FInventoryItem Prototype=Data->ToInventoryItem(1);
    if (!bIgnoreWeight && GetTotalWeight()+Prototype.Weight*Count > MaxWeight+KINDA_SMALL_NUMBER) return EInventoryResult::TooHeavy;
    if (!CanFitItem(Prototype,Count)) return EInventoryResult::InventoryFull;
    const TArray<FInventorySlot> OriginalSlots = Slots;
    int32 Remaining=Count;
    while (Remaining>0)
    {
        int32 Index=(Prototype.MaxStack>1)?FindStackSlot(ID):INDEX_NONE;
        if (Index==INDEX_NONE) Index=FindEmptySlot();
        if (Index==INDEX_NONE)
        {
            Slots = OriginalSlots;
            return EInventoryResult::InventoryFull;
        }
        if (!Slots[Index].IsOccupied()) { Slots[Index].Item=Data->ToInventoryItem(0); Slots[Index].SlotIndex=Index; }
        const int32 Space=FMath::Max(0,Slots[Index].Item.MaxStack-Slots[Index].Item.Count);
        const int32 Add=FMath::Min(Space,Remaining);
        if (Add<=0)
        {
            Slots = OriginalSlots;
            return EInventoryResult::InventoryFull;
        }
        Slots[Index].Item.Count+=Add; Remaining-=Add;
    }
    if (!bSuppressInventoryEvents && OnItemAdded.IsBound()) OnItemAdded.Broadcast(ID,GetItemCount(ID)); BroadcastChanged(); return EInventoryResult::Success;
}
EInventoryResult UInventoryComponent::AddItemData(const FInventoryItem& Item)
{
    if (Item.Count<=0 || Item.ItemID.IsNone()) return EInventoryResult::InvalidRequest;
    UInventoryItemData* D=GetItemData(Item.ItemID);
    if (D) return AddItem(Item.ItemID,Item.Count);
    // Fallback permits save/load of item records only when the slot has room; it does not invent definitions.
    if (!bIgnoreWeight && GetTotalWeight()+Item.Weight*Item.Count>MaxWeight+KINDA_SMALL_NUMBER) return EInventoryResult::TooHeavy;
    const int32 ActiveSlotCount = FMath::Clamp(MaxSlots, 0, Slots.Num());
    int32 Free=0; for (int32 i=0; i<ActiveSlotCount; ++i) if (!Slots[i].bLocked&&!Slots[i].IsOccupied()) ++Free;
    const int32 Stack=FMath::Max(1,Item.MaxStack); const int32 Need=FMath::DivideAndRoundUp(Item.Count,Stack);
    if (Need>Free) return EInventoryResult::InventoryFull;
    const TArray<FInventorySlot> OriginalSlots = Slots;
    int32 Left=Item.Count;
    while(Left>0)
    {
        int32 I=FindEmptySlot();
        if(I==INDEX_NONE)
        {
            Slots = OriginalSlots;
            return EInventoryResult::InventoryFull;
        }
        Slots[I].Item=Item; Slots[I].Item.Count=FMath::Min(Left,Stack); Slots[I].SlotIndex=I; Left-=Slots[I].Item.Count;
    }
    BroadcastChanged(); return EInventoryResult::Success;
}
EInventoryResult UInventoryComponent::RemoveItem(FName ID,int32 Count)
{
    if (Count<=0 || ID.IsNone()) return EInventoryResult::InvalidRequest;
    if (!HasItem(ID,Count)) return EInventoryResult::NotEnoughItems;
    int32 Left=Count;
    for (int32 i=Slots.Num()-1;i>=0&&Left>0;--i) { FInventorySlot& S=Slots[i]; if(S.bLocked||!S.IsOccupied()||S.Item.ItemID!=ID) continue; const int32 N=FMath::Min(S.Item.Count,Left); S.Item.Count-=N; Left-=N; if(S.Item.Count<=0) { S.Item=FInventoryItem(); S.SlotIndex=i; } }
    if(OnItemRemoved.IsBound()) OnItemRemoved.Broadcast(ID,GetItemCount(ID)); BroadcastChanged(); return EInventoryResult::Success;
}
EInventoryResult UInventoryComponent::DropItem(FName ID,int32 Count)
{
    UInventoryItemData* D=GetItemData(ID); if(!D||!D->bCanBeDropped) return EInventoryResult::InvalidRequest; return RemoveItem(ID,Count);
}
int32 UInventoryComponent::GetItemCount(FName ID) const { int32 N=0; for(const FInventorySlot& S:Slots) if(S.IsOccupied()&&S.Item.ItemID==ID) N+=S.Item.Count; return N; }
bool UInventoryComponent::HasItem(FName ID,int32 Count) const { return Count>=0 && GetItemCount(ID)>=Count; }
bool UInventoryComponent::HasAllItems(const TArray<FInventoryItem>& Req) const { for(const FInventoryItem& I:Req) if(!HasItem(I.ItemID,I.Count)) return false; return true; }
TArray<FInventorySlot> UInventoryComponent::GetSlotsByCategory(EInventoryCategory C) const { TArray<FInventorySlot> R; for(const FInventorySlot& S:Slots) if(S.IsOccupied()&&S.Item.Category==C) R.Add(S); return R; }
int32 UInventoryComponent::GetUsedSlotCount() const
{
    const int32 ActiveSlotCount = FMath::Clamp(MaxSlots, 0, Slots.Num());
    int32 N=0;
    for (int32 i=0; i<ActiveSlotCount; ++i) if (Slots[i].IsOccupied()) ++N;
    return N;
}
float UInventoryComponent::GetTotalWeight() const { float W=0; for(const FInventorySlot& S:Slots) if(S.IsOccupied()) W+=S.Item.Weight*S.Item.Count; return W; }
void UInventoryComponent::NormalizeSlotIndexes() { for(int32 i=0;i<Slots.Num();++i) Slots[i].SlotIndex=i; }
void UInventoryComponent::SortByCategory() { Slots.Sort([](const FInventorySlot&A,const FInventorySlot&B){if(A.IsOccupied()!=B.IsOccupied())return A.IsOccupied(); return (uint8)A.Item.Category<(uint8)B.Item.Category;}); NormalizeSlotIndexes(); BroadcastChanged(); }
void UInventoryComponent::SortByName() { Slots.Sort([](const FInventorySlot&A,const FInventorySlot&B){if(A.IsOccupied()!=B.IsOccupied())return A.IsOccupied();return A.Item.DisplayName.ToString()<B.Item.DisplayName.ToString();}); NormalizeSlotIndexes(); BroadcastChanged(); }
void UInventoryComponent::SortByRarity() { Slots.Sort([](const FInventorySlot&A,const FInventorySlot&B){if(A.IsOccupied()!=B.IsOccupied())return A.IsOccupied();return (uint8)A.Item.Rarity>(uint8)B.Item.Rarity;}); NormalizeSlotIndexes(); BroadcastChanged(); }
void UInventoryComponent::ClearAll() { for(int32 i=0;i<Slots.Num();++i){ if(!Slots[i].bLocked){Slots[i].Item=FInventoryItem();Slots[i].SlotIndex=i;} } BroadcastChanged(); }
bool UInventoryComponent::RestoreFromSavedItems(const TArray<FInventoryItem>& Saved)
{
    const TArray<FInventorySlot> OriginalSlots = Slots;
    const bool bWasSuppressingEvents = bSuppressInventoryEvents;
    bSuppressInventoryEvents = true;
    ClearAll();

    float Weight = 0.0f;
    int32 Needed = 0;
    for (const FInventoryItem& Item : Saved)
    {
        if (Item.IsEmpty()) continue;
        if (Item.Count < 0 || Item.MaxStack <= 0 || Item.Weight < 0.0f)
        {
            Slots = OriginalSlots;
            NormalizeSlotIndexes();
            bSuppressInventoryEvents = bWasSuppressingEvents;
            BroadcastChanged();
            return false;
        }
        Weight += Item.Weight * Item.Count;
        Needed += FMath::DivideAndRoundUp(Item.Count, FMath::Max(1, Item.MaxStack));
    }

    const int32 ActiveSlotCount = FMath::Clamp(MaxSlots, 0, Slots.Num());
    int32 Free = 0;
    for (int32 i = 0; i < ActiveSlotCount; ++i)
    {
        if (!Slots[i].bLocked && !Slots[i].IsOccupied()) ++Free;
    }

    if (Needed > Free || (!bIgnoreWeight && Weight > MaxWeight + KINDA_SMALL_NUMBER))
    {
        Slots = OriginalSlots;
        NormalizeSlotIndexes();
        bSuppressInventoryEvents = bWasSuppressingEvents;
        BroadcastChanged();
        return false;
    }

    for (const FInventoryItem& Item : Saved)
    {
        if (Item.IsEmpty()) continue;
        if (AddItemData(Item) != EInventoryResult::Success)
        {
            Slots = OriginalSlots;
            NormalizeSlotIndexes();
            bSuppressInventoryEvents = bWasSuppressingEvents;
            BroadcastChanged();
            return false;
        }
    }

    bSuppressInventoryEvents = bWasSuppressingEvents;
    BroadcastChanged();
    return true;
}
void UInventoryComponent::BroadcastChanged() { if(!bSuppressInventoryEvents && OnInventoryChanged.IsBound()) OnInventoryChanged.Broadcast(this); }
