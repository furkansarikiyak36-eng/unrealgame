#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InventoryCore/InventoryComponent.h"
#include "InventoryStorageActor.generated.h"

class UStaticMeshComponent;

/**
 * Placeable stockpile/container that uses the same canonical inventory rules as
 * the player. Its key should be configured for runtime-spawned containers; a
 * level-placed container falls back to its stable actor name.
 */
UCLASS()
class MYPROJECT2_API AInventoryStorageActor : public AActor
{
    GENERATED_BODY()

public:
    AInventoryStorageActor();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintPure, Category = "Inventory|Storage")
    UInventoryComponent* GetInventory() const { return Inventory; }

    UFUNCTION(BlueprintPure, Category = "Inventory|Storage")
    FName GetStorageKey() const { return StorageKey.IsNone() ? GetFName() : StorageKey; }

    UFUNCTION(BlueprintCallable, Category = "Inventory|Storage")
    bool DepositFrom(UInventoryComponent* Source, FName ItemID, int32 Count = 1);

    UFUNCTION(BlueprintCallable, Category = "Inventory|Storage")
    bool WithdrawTo(UInventoryComponent* Destination, FName ItemID, int32 Count = 1);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Storage")
    TObjectPtr<UInventoryComponent> Inventory;

    UPROPERTY(EditInstanceOnly, SaveGame, BlueprintReadOnly, Category = "Inventory|Storage")
    FName StorageKey = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Storage")
    TObjectPtr<UStaticMeshComponent> Mesh;
};
