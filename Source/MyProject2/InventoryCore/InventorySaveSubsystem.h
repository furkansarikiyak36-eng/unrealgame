#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "InventoryTypes.h"
#include "InventorySaveSubsystem.generated.h"
class UInventoryComponent;
UCLASS()
class MYPROJECT2_API UInventorySaveGame : public USaveGame
{
 GENERATED_BODY()
public:
 UPROPERTY() TArray<FInventoryItem> Items;
 UPROPERTY() TArray<FName> QuickSlots;
 UPROPERTY() int32 ActiveQuickSlot=INDEX_NONE;
};
UCLASS()
class MYPROJECT2_API UInventorySaveSubsystem : public UGameInstanceSubsystem
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable) bool SaveInventory(UInventoryComponent* Inventory,const FString& SlotName);
 UFUNCTION(BlueprintCallable) bool LoadInventory(UInventoryComponent* Inventory,const FString& SlotName);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 UserIndex=0;
};
