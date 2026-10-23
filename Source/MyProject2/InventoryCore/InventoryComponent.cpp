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
    for (int32 i=0;i<Slots.Num();++i) if (!Slots[i].bLocked && Slots[i].IsOccupied() && Slots[i].Item.ItemID==ID && !Slots[i].Item.IsFull()) return i;
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
    int32 Space=0, NeededSlots=0;
    if (Item.MaxStack>1)
        for (const FInventorySlot& S:Slots) if (!S.bLocked && S.IsOccupied() && S.Item.ItemID==Item.ItemID) Space+=FMath::Max(0,S.Item.MaxStack-S.Item.Count);
    const int32 Remaining=FMath::Max(0,Count-Space);
    NeededSlots = Item.MaxStack>1 ? FMath::DivideAndRoundUp(Remaining,FMath::Max(1,Item.MaxStack)) : Remaining;
    int32 Free=0; for (const FInventorySlot& S:Slots) if (!S.bLocked && !S.IsOccupied()) ++Free;
    return Slots.Num() >= MaxSlots && NeededSlots <= Free;
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
    int32 Remaining=Count;
    while (Remaining>0)
    {
        int32 Index=(Prototype.MaxStack>1)?FindStackSlot(ID):INDEX_NONE;
        if (Index==INDEX_NONE) Index=FindEmptySlot();
        if (Index==INDEX_NONE) return EInventoryResult::InventoryFull; // preflight should make this unreachable
        if (!Slots[Index].IsOccupied()) { Slots[Index].Item=Data->ToInventoryItem(0); Slots[Index].SlotIndex=Index; }
        const int32 Space=FMath::Max(0,Slots[Index].Item.MaxStack-Slots[Index].Item.Count);
        const int32 Add=FMath::Min(Space,Remaining);
        if (Add<=0) return EInventoryResult::InventoryFull;
        Slots[Index].Item.Count+=Add; Remaining-=Add;
    }
    if (OnItemAdded.IsBound()) OnItemAdded.Broadcast(ID,GetItemCount(ID)); BroadcastChanged(); return EInventoryResult::Success;
}
EInventoryResult UInventoryComponent::AddItemData(const FInventoryItem& Item)
{
    if (Item.Count<=0 || Item.ItemID.IsNone()) return EInventoryResult::InvalidRequest;
    UInventoryItemData* D=GetItemData(Item.ItemID);
    if (D) return AddItem(Item.ItemID,Item.Count);
    // Fallback permits save/load of item records only when the slot has room; it does not invent definitions.
    if (!bIgnoreWeight && GetTotalWeight()+Item.Weight*Item.Count>MaxWeight+KINDA_SMALL_NUMBER) return EInventoryResult::TooHeavy;
    int32 Free=0; for (const FInventorySlot& S:Slots) if (!S.bLocked&&!S.IsOccupied()) ++Free;
    const int32 Stack=FMath::Max(1,Item.MaxStack); const int32 Need=FMath::DivideAndRoundUp(Item.Count,Stack);
    if (Need>Free) return EInventoryResult::InventoryFull;
    int32 Left=Item.Count;
    while(Left>0) { int32 I=FindEmptySlot(); if(I==INDEX_NONE) return EInventoryResult::InventoryFull; Slots[I].Item=Item; Slots[I].Item.Count=FMath::Min(Left,Stack); Slots[I].SlotIndex=I; Left-=Slots[I].Item.Count; }
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
int32 UInventoryComponent::GetUsedSlotCount() const { int32 N=0; for(const FInventorySlot& S:Slots) if(S.IsOccupied()) ++N; return N; }
float UInventoryComponent::GetTotalWeight() const { float W=0; for(const FInventorySlot& S:Slots) if(S.IsOccupied()) W+=S.Item.Weight*S.Item.Count; return W; }
void UInventoryComponent::NormalizeSlotIndexes() { for(int32 i=0;i<Slots.Num();++i) Slots[i].SlotIndex=i; }
void UInventoryComponent::SortByCategory() { Slots.Sort([](const FInventorySlot&A,const FInventorySlot&B){if(A.IsOccupied()!=B.IsOccupied())return A.IsOccupied(); return (uint8)A.Item.Category<(uint8)B.Item.Category;}); NormalizeSlotIndexes(); BroadcastChanged(); }
void UInventoryComponent::SortByName() { Slots.Sort([](const FInventorySlot&A,const FInventorySlot&B){if(A.IsOccupied()!=B.IsOccupied())return A.IsOccupied();return A.Item.DisplayName.ToString()<B.Item.DisplayName.ToString();}); NormalizeSlotIndexes(); BroadcastChanged(); }
void UInventoryComponent::SortByRarity() { Slots.Sort([](const FInventorySlot&A,const FInventorySlot&B){if(A.IsOccupied()!=B.IsOccupied())return A.IsOccupied();return (uint8)A.Item.Rarity>(uint8)B.Item.Rarity;}); NormalizeSlotIndexes(); BroadcastChanged(); }
void UInventoryComponent::ClearAll() { for(int32 i=0;i<Slots.Num();++i){ if(!Slots[i].bLocked){Slots[i].Item=FInventoryItem();Slots[i].SlotIndex=i;} } BroadcastChanged(); }
bool UInventoryComponent::RestoreFromSavedItems(const TArray<FInventoryItem>& Saved)
{
	if (MaxSlots <= 0 || Slots.Num() != MaxSlots || !FMath::IsFinite(MaxWeight) || MaxWeight < 0.f)
	{
		return false;
	}

	// Build the restored inventory off to the side. Locked slots retain their
	// existing contents and flags, matching ClearAll's established behavior.
	TArray<FInventorySlot> StagedSlots = Slots;
	for (int32 Index = 0; Index < StagedSlots.Num(); ++Index)
	{
		if (!StagedSlots[Index].bLocked)
		{
			StagedSlots[Index].Item = FInventoryItem();
			StagedSlots[Index].SlotIndex = Index;
		}
	}

	// Reject invalid locked records before treating them as the baseline.
	for (const FInventorySlot& Slot : StagedSlots)
	{
		if (!Slot.bLocked || !Slot.IsOccupied())
		{
			continue;
		}
		const FInventoryItem& Item = Slot.Item;
		if (Item.ItemID.IsNone() || Item.Count <= 0 || Item.MaxStack <= 0 ||
			Item.Count > Item.MaxStack || !FMath::IsFinite(Item.Weight) || Item.Weight < 0.f)
		{
			return false;
		}
	}

	for (const FInventoryItem& SavedItem : Saved)
	{
		// Empty serialized slots are allowed; a positive count without an ID is not.
		if (SavedItem.Count < 0 || (SavedItem.Count > 0 && SavedItem.ItemID.IsNone()))
		{
			return false;
		}
		if (SavedItem.IsEmpty())
		{
			continue;
		}
		if (SavedItem.MaxStack <= 0 || !FMath::IsFinite(SavedItem.Weight) || SavedItem.Weight < 0.f)
		{
			return false;
		}

		UInventoryItemData* Definition = GetItemData(SavedItem.ItemID);
		FInventoryItem Prototype = Definition ? Definition->ToInventoryItem(1) : SavedItem;
		if (Prototype.ItemID.IsNone() || Prototype.MaxStack <= 0 ||
			!FMath::IsFinite(Prototype.Weight) || Prototype.Weight < 0.f)
		{
			return false;
		}
		Prototype.Count = 0;

		int32 Remaining = SavedItem.Count;
		while (Remaining > 0)
		{
			int32 SlotIndex = INDEX_NONE;
			if (Prototype.MaxStack > 1)
			{
				for (int32 Index = 0; Index < MaxSlots; ++Index)
				{
					const FInventorySlot& Slot = StagedSlots[Index];
					if (!Slot.bLocked && Slot.IsOccupied() &&
						Slot.Item.ItemID == Prototype.ItemID &&
						Slot.Item.Count < Slot.Item.MaxStack)
					{
						SlotIndex = Index;
						break;
					}
				}
			}

			if (SlotIndex == INDEX_NONE)
			{
				for (int32 Index = 0; Index < MaxSlots; ++Index)
				{
					if (!StagedSlots[Index].bLocked && !StagedSlots[Index].IsOccupied())
					{
						SlotIndex = Index;
						break;
					}
				}
			}

			if (SlotIndex == INDEX_NONE)
			{
				return false;
			}

			FInventorySlot& TargetSlot = StagedSlots[SlotIndex];
			if (!TargetSlot.IsOccupied())
			{
				TargetSlot.Item = Prototype;
				TargetSlot.SlotIndex = SlotIndex;
			}

			const int32 StackCapacity = FMath::Max(1, TargetSlot.Item.MaxStack);
			const int32 Space = FMath::Max(0, StackCapacity - TargetSlot.Item.Count);
			const int32 AmountToAdd = FMath::Min(Space, Remaining);
			if (AmountToAdd <= 0)
			{
				return false;
			}

			TargetSlot.Item.Count += AmountToAdd;
			Remaining -= AmountToAdd;
		}
	}

	double TotalWeight = 0.0;
	for (const FInventorySlot& Slot : StagedSlots)
	{
		if (!Slot.IsOccupied())
		{
			continue;
		}
		const FInventoryItem& Item = Slot.Item;
		if (Item.Count <= 0 || Item.ItemID.IsNone() || Item.MaxStack <= 0 ||
			Item.Count > Item.MaxStack || !FMath::IsFinite(Item.Weight) || Item.Weight < 0.f)
		{
			return false;
		}
		TotalWeight += static_cast<double>(Item.Weight) * static_cast<double>(Item.Count);
		if (!FMath::IsFinite(TotalWeight))
		{
			return false;
		}
	}

	if (!bIgnoreWeight && TotalWeight > static_cast<double>(MaxWeight) + static_cast<double>(KINDA_SMALL_NUMBER))
	{
		return false;
	}

	// Single state commit. No broadcast/delegate can observe a partial restore.
	Slots = MoveTemp(StagedSlots);
	NormalizeSlotIndexes();
	BroadcastChanged();
	return true;
}
void UInventoryComponent::BroadcastChanged() { if(OnInventoryChanged.IsBound()) OnInventoryChanged.Broadcast(this); }
