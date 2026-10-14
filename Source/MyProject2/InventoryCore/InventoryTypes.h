#pragma once
#include "CoreMinimal.h"
#include "InventoryCategory.h"
#include "InventoryTypes.generated.h"

class UTexture2D;

class UInventoryComponent;

USTRUCT(BlueprintType)
struct FInventoryItem
{
    GENERATED_BODY()
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FName ItemID = NAME_None;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FText DisplayName;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) FText Description;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) EInventoryCategory Category = EInventoryCategory::Resource;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) EItemRarity Rarity = EItemRarity::Common;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) int32 Count = 0;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) int32 MaxStack = 20;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) float Weight = 0.1f;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) bool bConsumable = false;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) bool bQuestItem = false;
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UTexture2D> Icon;
    bool IsFull() const { return Count >= MaxStack; }
    bool IsEmpty() const { return Count <= 0 || ItemID.IsNone(); }
};

USTRUCT(BlueprintType)
struct FInventorySlot
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) int32 SlotIndex = INDEX_NONE;
    UPROPERTY(BlueprintReadWrite) FInventoryItem Item;
    UPROPERTY(BlueprintReadWrite) bool bLocked = false;
    UPROPERTY(BlueprintReadWrite) bool bHighlighted = false;
    bool IsOccupied() const { return !Item.IsEmpty(); }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryChanged, UInventoryComponent*, Inventory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAdded, FName, ItemID, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemRemoved, FName, ItemID, int32, RemainingCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemEquipped, FName, ItemID, int32, SlotIndex);
