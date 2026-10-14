#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryTypes.h"
#include "InventoryComponent.generated.h"

class UInventoryItemData;

UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UInventoryComponent();
    virtual void BeginPlay() override;
    UFUNCTION(BlueprintCallable, Category="Inventory") EInventoryResult AddItem(FName ItemID, int32 Count=1);
    UFUNCTION(BlueprintCallable, Category="Inventory") EInventoryResult AddItemData(const FInventoryItem& Item);
    UFUNCTION(BlueprintCallable, Category="Inventory") EInventoryResult RemoveItem(FName ItemID, int32 Count=1);
    UFUNCTION(BlueprintCallable, Category="Inventory") EInventoryResult DropItem(FName ItemID, int32 Count=1);
    // Atomic in-memory transfer: either both inventories reflect the transfer or source stock is restored.
    UFUNCTION(BlueprintCallable, Category="Inventory|Transactions") static bool TransferItem(UInventoryComponent* Source, UInventoryComponent* Destination, FName ItemID, int32 Count=1);
    UFUNCTION(BlueprintPure, Category="Inventory") int32 GetItemCount(FName ItemID) const;
    UFUNCTION(BlueprintPure, Category="Inventory") bool HasItem(FName ItemID, int32 Count=1) const;
    UFUNCTION(BlueprintPure, Category="Inventory") bool HasAllItems(const TArray<FInventoryItem>& Required) const;
    UFUNCTION(BlueprintPure, Category="Inventory") TArray<FInventorySlot> GetSlots() const { return Slots; }
    UFUNCTION(BlueprintPure, Category="Inventory") TArray<FInventorySlot> GetSlotsByCategory(EInventoryCategory Category) const;
    UFUNCTION(BlueprintPure, Category="Inventory") int32 GetUsedSlotCount() const;
    UFUNCTION(BlueprintPure, Category="Inventory") int32 GetFreeSlotCount() const { return FMath::Max(0, FMath::Min(MaxSlots, Slots.Num())-GetUsedSlotCount()); }
    UFUNCTION(BlueprintPure, Category="Inventory") bool IsFull() const { return GetFreeSlotCount() <= 0; }
    UFUNCTION(BlueprintPure, Category="Inventory") float GetTotalWeight() const;
    UFUNCTION(BlueprintCallable, Category="Inventory") bool CanAddItem(FName ItemID, int32 Count=1) const;
    UFUNCTION(BlueprintCallable, Category="Inventory") void SortByCategory();
    UFUNCTION(BlueprintCallable, Category="Inventory") void SortByName();
    UFUNCTION(BlueprintCallable, Category="Inventory") void SortByRarity();
    UFUNCTION(BlueprintCallable, Category="Inventory") void ClearAll();
    UFUNCTION(BlueprintCallable, Category="Save") bool RestoreFromSavedItems(const TArray<FInventoryItem>& SavedItems);
    UPROPERTY(BlueprintAssignable, Category="Events") FOnInventoryChanged OnInventoryChanged;
    UPROPERTY(BlueprintAssignable, Category="Events") FOnItemAdded OnItemAdded;
    UPROPERTY(BlueprintAssignable, Category="Events") FOnItemRemoved OnItemRemoved;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config") int32 MaxSlots=40;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config") float MaxWeight=100.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Config") bool bIgnoreWeight=false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Definitions") TArray<TObjectPtr<UInventoryItemData>> ItemDefinitions;
protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="State") TArray<FInventorySlot> Slots;
    UInventoryItemData* GetItemData(FName ItemID) const;
    int32 FindStackSlot(FName ItemID) const;
    int32 FindEmptySlot() const;
    void BroadcastChanged();
    bool CanFitItem(const FInventoryItem& Item, int32 Count) const;
    void NormalizeSlotIndexes();
    bool bSuppressInventoryEvents = false;
};
