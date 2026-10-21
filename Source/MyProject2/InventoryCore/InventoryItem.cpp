#include "InventoryItem.h"

TMap<FName, TWeakObjectPtr<UInventoryItemData>> UInventoryItemLibrary::ItemRegistry;

bool UInventoryItemData::ValidateData(FString& OutError) const
{
    if (ItemID.IsNone()) { OutError = TEXT("ItemID boş"); return false; }
    if (MaxStack <= 0) { OutError = TEXT("MaxStack <= 0"); return false; }
    if (!FMath::IsFinite(Weight) || Weight < 0.0f) { OutError = TEXT("Weight must be finite and >= 0"); return false; }
    if (!bStackable && MaxStack != 1) { OutError = TEXT("Stackable olmayan eşyanın MaxStack değeri 1 olmalı"); return false; }
    OutError.Empty(); return true;
}

FInventoryItem UInventoryItemData::ToInventoryItem(int32 InCount) const
{
    FInventoryItem Item; Item.ItemID=ItemID; Item.DisplayName=DisplayName; Item.Description=Description;
    Item.Category=Category; Item.Rarity=Rarity; Item.Count=FMath::Max(0, InCount);
    Item.MaxStack=bStackable ? FMath::Max(1, MaxStack) : 1; Item.Weight=FMath::Max(0.0f, Weight);
    Item.bConsumable=bConsumable; Item.bQuestItem=bQuestItem; Item.Icon=Icon; return Item;
}

UInventoryItemData* UInventoryItemLibrary::FindItemData(FName ItemID)
{
    if (const TWeakObjectPtr<UInventoryItemData>* Found=ItemRegistry.Find(ItemID)) return Found->Get();
    return nullptr;
}
void UInventoryItemLibrary::RegisterItemData(UInventoryItemData* ItemData)
{
    if (!ItemData) return;
    FString ValidationError;
    if (!ItemData->ValidateData(ValidationError))
    {
        UE_LOG(LogTemp, Error, TEXT("Inventory item definition '%s' rejected: %s"),
            *ItemData->GetName(), *ValidationError);
        return;
    }
    ItemRegistry.Add(ItemData->ItemID, ItemData);
}
TArray<UInventoryItemData*> UInventoryItemLibrary::GetAllItems()
{
    TArray<UInventoryItemData*> Result;
    for (auto It=ItemRegistry.CreateIterator(); It; ++It)
    {
        if (It.Value().IsValid()) Result.Add(It.Value().Get()); else It.RemoveCurrent();
    }
    return Result;
}
