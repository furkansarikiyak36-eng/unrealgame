#include "InventoryComponent.h"
#include "InventoryItem.h"

UInventoryComponent::UInventoryComponent() { PrimaryComponentTick.bCanEverTick=false; }
void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeRuntimeInventory();
}

void UInventoryComponent::InitializeRuntimeInventory()
{
    MaxSlots = FMath::Max(1, MaxSlots);
    // This can be called again by the owning character after injecting its canonical
    // item definitions. Never resize a live inventory here and risk truncating items.
    if (Slots.Num() == 0)
    {
        Slots.SetNum(MaxSlots);
    }
    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        Slots[i].SlotIndex = i;
    }

    for (UInventoryItemData* D : ItemDefinitions)
    {
        if (!D) continue;
        FString ValidationError;
        if (!D->ValidateData(ValidationError))
        {
            UE_LOG(LogTemp, Error, TEXT("Inventory item definition '%s' rejected on owner '%s': %s"),
                *D->GetName(), *GetNameSafe(GetOwner()), *ValidationError);
            continue;
        }
        UInventoryItemLibrary::RegisterItemData(D);
    }
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
    UInventoryItemData* D = GetItemData(ID);
    if (!D) return false;
    FString ValidationError;
    if (!D->ValidateData(ValidationError)) return false;
    return CanFitItem(D->ToInventoryItem(1), Count);
}
EInventoryResult UInventoryComponent::AddItem(FName ID, int32 Count)
{
    if (Count<=0 || ID.IsNone()) return EInventoryResult::InvalidRequest;
    UInventoryItemData* Data=GetItemData(ID);
    if (!Data) return EInventoryResult::NoRecipeMatch;
    FString ValidationError;
    if (!Data->ValidateData(ValidationError))
    {
        UE_LOG(LogTemp, Error, TEXT("AddItem rejected invalid definition '%s': %s"),
            *Data->GetName(), *ValidationError);
        return EInventoryResult::InvalidRequest;
    }
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
    if (Item.Count<=0 || Item.ItemID.IsNone() || Item.MaxStack <= 0 ||
        !FMath::IsFinite(Item.Weight) || Item.Weight < 0.0f)
    {
        return EInventoryResult::InvalidRequest;
    }
    UInventoryItemData* D=GetItemData(Item.ItemID);
    if (D)
    {
        FString ValidationError;
        if (!D->ValidateData(ValidationError))
        {
            UE_LOG(LogTemp, Error, TEXT("AddItemData rejected invalid definition '%s': %s"),
                *D->GetName(), *ValidationError);
            return EInventoryResult::InvalidRequest;
        }
        return AddItem(Item.ItemID,Item.Count);
    }
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
EInventoryResult UInventoryComponent::RemoveItem(FName ID, int32 Count)
{
    if (Count <= 0 || ID.IsNone())
    {
        return EInventoryResult::InvalidRequest;
    }
    if (!HasItem(ID, Count))
    {
        return EInventoryResult::NotEnoughItems;
    }

    int32 Left = Count;
    for (int32 i = Slots.Num() - 1; i >= 0 && Left > 0; --i)
    {
        FInventorySlot& Slot = Slots[i];
        if (Slot.bLocked || !Slot.IsOccupied() || Slot.Item.ItemID != ID) continue;
        const int32 Removed = FMath::Min(Slot.Item.Count, Left);
        Slot.Item.Count -= Removed;
        Left -= Removed;
        if (Slot.Item.Count <= 0)
        {
            Slot.Item = FInventoryItem();
            Slot.SlotIndex = i;
        }
    }

    if (!bSuppressInventoryEvents && OnItemRemoved.IsBound())
    {
        OnItemRemoved.Broadcast(ID, GetItemCount(ID));
    }
    BroadcastChanged();
    return EInventoryResult::Success;
}
bool UInventoryComponent::CanDropItem(FName ID, int32 Count) const
{
    const UInventoryItemData* Definition = GetItemData(ID);
    return Count > 0 && !ID.IsNone() && Definition && Definition->bCanBeDropped && HasItem(ID, Count);
}

EInventoryResult UInventoryComponent::DropItem(FName ID, int32 Count)
{
    if (!CanDropItem(ID, Count))
    {
        return EInventoryResult::InvalidRequest;
    }
    return RemoveItem(ID, Count);
}
bool UInventoryComponent::TransferItem(UInventoryComponent* Source, UInventoryComponent* Destination, FName ItemID, int32 Count)
{
    if (!IsValid(Source) || !IsValid(Destination) || Source == Destination ||
        ItemID.IsNone() || Count <= 0 || !Source->HasItem(ItemID, Count))
    {
        return false;
    }

    FInventoryItem Payload;
    bool bFoundPayload = false;
    for (const FInventorySlot& Slot : Source->Slots)
    {
        if (Slot.IsOccupied() && Slot.Item.ItemID == ItemID)
        {
            Payload = Slot.Item;
            bFoundPayload = true;
            break;
        }
    }
    if (!bFoundPayload)
    {
        return false;
    }

    Payload.Count = Count;
    if (!Destination->CanAddItem(ItemID, Count))
    {
        // Save-derived item rows without a definition can transfer only if their
        // payload fits the destination's same weight/slot policy.
        if (Destination->GetItemData(ItemID) != nullptr ||
            !Destination->CanFitItem(Payload, Count))
        {
            return false;
        }
    }

    // Preserve both exact slot layouts. Suppress delegates until both sides commit;
    // a consumer must never observe the temporary debit between Remove and Add.
    const TArray<FInventorySlot> OriginalSourceSlots = Source->Slots;
    const TArray<FInventorySlot> OriginalDestinationSlots = Destination->Slots;
    const bool bWasSuppressingSourceEvents = Source->bSuppressInventoryEvents;
    const bool bWasSuppressingDestinationEvents = Destination->bSuppressInventoryEvents;
    Source->bSuppressInventoryEvents = true;
    Destination->bSuppressInventoryEvents = true;

    const EInventoryResult Removed = Source->RemoveItem(ItemID, Count);
    const EInventoryResult Added = Removed == EInventoryResult::Success
        ? Destination->AddItemData(Payload)
        : EInventoryResult::InvalidRequest;

    if (Removed != EInventoryResult::Success || Added != EInventoryResult::Success)
    {
        Source->Slots = OriginalSourceSlots;
        Destination->Slots = OriginalDestinationSlots;
        Source->NormalizeSlotIndexes();
        Destination->NormalizeSlotIndexes();
        Source->bSuppressInventoryEvents = bWasSuppressingSourceEvents;
        Destination->bSuppressInventoryEvents = bWasSuppressingDestinationEvents;
        return false;
    }

    Source->bSuppressInventoryEvents = bWasSuppressingSourceEvents;
    Destination->bSuppressInventoryEvents = bWasSuppressingDestinationEvents;
    if (!bWasSuppressingSourceEvents)
    {
        if (Source->OnItemRemoved.IsBound())
        {
            Source->OnItemRemoved.Broadcast(ItemID, Source->GetItemCount(ItemID));
        }
        Source->BroadcastChanged();
    }
    if (!bWasSuppressingDestinationEvents)
    {
        if (Destination->OnItemAdded.IsBound())
        {
            Destination->OnItemAdded.Broadcast(ItemID, Destination->GetItemCount(ItemID));
        }
        Destination->BroadcastChanged();
    }
    return true;
}

bool UInventoryComponent::ExchangeItems(FName InputID, int32 InputCount, FName OutputID, int32 OutputCount)
{
    if (InputID.IsNone() || OutputID.IsNone() || InputID == OutputID || InputCount <= 0 || OutputCount <= 0 ||
        !HasItem(InputID, InputCount) || !GetItemData(InputID) || !GetItemData(OutputID))
    {
        return false;
    }

    const TArray<FInventorySlot> OriginalSlots = Slots;
    const bool bWasSuppressingEvents = bSuppressInventoryEvents;
    bSuppressInventoryEvents = true;

    const EInventoryResult Removed = RemoveItem(InputID, InputCount);
    const EInventoryResult Added = Removed == EInventoryResult::Success
        ? AddItem(OutputID, OutputCount)
        : EInventoryResult::InvalidRequest;

    if (Removed != EInventoryResult::Success || Added != EInventoryResult::Success)
    {
        Slots = OriginalSlots;
        NormalizeSlotIndexes();
        bSuppressInventoryEvents = bWasSuppressingEvents;
        return false;
    }

    bSuppressInventoryEvents = bWasSuppressingEvents;
    if (!bWasSuppressingEvents)
    {
        if (OnItemRemoved.IsBound())
        {
            OnItemRemoved.Broadcast(InputID, GetItemCount(InputID));
        }
        if (OnItemAdded.IsBound())
        {
            OnItemAdded.Broadcast(OutputID, GetItemCount(OutputID));
        }
        BroadcastChanged();
    }
    return true;
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
bool UInventoryComponent::CanRestoreFromSavedItems(const TArray<FInventoryItem>& Saved) const
{
    float TotalWeight = 0.0f;
    int64 NeededSlots = 0;
    TMap<FName, int64> DefinedCounts;
    TMap<FName, int32> DefinedStackSizes;

    const int32 ActiveSlotCount = FMath::Clamp(MaxSlots, 0, Slots.Num());
    int32 AvailableSlots = 0;
    for (int32 i = 0; i < ActiveSlotCount; ++i)
    {
        const FInventorySlot& Slot = Slots[i];
        if (!Slot.bLocked)
        {
            // Restore clears all unlocked slots before filling the snapshot.
            ++AvailableSlots;
        }
        else if (Slot.IsOccupied())
        {
            TotalWeight += Slot.Item.Weight * Slot.Item.Count;
        }
    }

    for (const FInventoryItem& Item : Saved)
    {
        if (Item.Count < 0)
        {
            return false;
        }
        if (Item.Count == 0)
        {
            continue;
        }
        if (Item.ItemID.IsNone() || Item.MaxStack <= 0 ||
            !FMath::IsFinite(Item.Weight) || Item.Weight < 0.0f)
        {
            return false;
        }

        UInventoryItemData* Definition = GetItemData(Item.ItemID);
        if (Definition)
        {
            FString ValidationError;
            if (!Definition->ValidateData(ValidationError))
            {
                return false;
            }
            int64& CombinedCount = DefinedCounts.FindOrAdd(Item.ItemID);
            if (CombinedCount > TNumericLimits<int64>::Max() - Item.Count)
            {
                return false;
            }
            CombinedCount += Item.Count;
            DefinedStackSizes.Add(Item.ItemID,
                Definition->bStackable ? FMath::Max(1, Definition->MaxStack) : 1);
            TotalWeight += Definition->Weight * Item.Count;
        }
        else
        {
            NeededSlots += (static_cast<int64>(Item.Count) + Item.MaxStack - 1) / Item.MaxStack;
            TotalWeight += Item.Weight * Item.Count;
        }

        if (NeededSlots > AvailableSlots || !FMath::IsFinite(TotalWeight))
        {
            return false;
        }
    }

    for (const TPair<FName, int64>& Entry : DefinedCounts)
    {
        const int32 StackSize = DefinedStackSizes.FindRef(Entry.Key);
        NeededSlots += (Entry.Value + StackSize - 1) / StackSize;
        if (NeededSlots > AvailableSlots)
        {
            return false;
        }
    }

    return bIgnoreWeight || TotalWeight <= MaxWeight + KINDA_SMALL_NUMBER;
}

bool UInventoryComponent::RestoreFromSavedItems(const TArray<FInventoryItem>& Saved)
{
    // Use the same validator and stack definitions as preflight; do not run a
    // second, weaker capacity calculation after the live inventory is cleared.
    if (!CanRestoreFromSavedItems(Saved))
    {
        return false;
    }

    const TArray<FInventorySlot> OriginalSlots = Slots;
    const bool bWasSuppressingEvents = bSuppressInventoryEvents;
    bSuppressInventoryEvents = true;
    ClearAll();

    auto RejectRestore = [this, &OriginalSlots, bWasSuppressingEvents]() -> bool
    {
        Slots = OriginalSlots;
        NormalizeSlotIndexes();
        bSuppressInventoryEvents = bWasSuppressingEvents;
        BroadcastChanged();
        return false;
    };

    for (const FInventoryItem& Item : Saved)
    {
        if (Item.Count == 0)
        {
            continue;
        }
        if (AddItemData(Item) != EInventoryResult::Success)
        {
            return RejectRestore();
        }
    }

    bSuppressInventoryEvents = bWasSuppressingEvents;
    BroadcastChanged();
    return true;
}
void UInventoryComponent::BroadcastChanged() { if(!bSuppressInventoryEvents && OnInventoryChanged.IsBound()) OnInventoryChanged.Broadcast(this); }
