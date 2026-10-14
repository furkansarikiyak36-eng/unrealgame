#include "InventoryToPhysicalBridge.h"

#include "InventoryCore/InventoryComponent.h"
#include "Phase1_Physical/LogActor.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StickPhaseS1_Core/StickActor.h"
#include "Engine/World.h"

UInventoryToPhysicalBridge::UInventoryToPhysicalBridge()
{
    PrimaryComponentTick.bCanEverTick = false;
}

UInventoryComponent* UInventoryToPhysicalBridge::GetInventory() const
{
    return GetOwner() ? GetOwner()->FindComponentByClass<UInventoryComponent>() : nullptr;
}

template <typename T>
static T* SpawnAfterInventoryCheck(
    UInventoryToPhysicalBridge* Self,
    FName ItemID,
    const FVector& Location,
    TSubclassOf<T> ActorClass)
{
    if (!Self || ItemID.IsNone() || !ActorClass)
    {
        return nullptr;
    }

    UWorld* World = Self->GetWorld();
    AActor* Owner = Self->GetOwner();
    UInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UInventoryComponent>() : nullptr;
    if (!World || !Inventory || !Inventory->HasItem(ItemID, 1))
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    T* SpawnedActor = World->SpawnActor<T>(
        ActorClass,
        Location,
        FRotator::ZeroRotator,
        SpawnParameters);
    if (!SpawnedActor)
    {
        return nullptr;
    }

    // DropItem enforces the item's bCanBeDropped policy. Spawn first, then
    // consume inventory; if the inventory operation fails, roll the spawn back.
    if (Inventory->DropItem(ItemID, 1) != EInventoryResult::Success)
    {
        SpawnedActor->Destroy();
        return nullptr;
    }

    return SpawnedActor;
}

ALogActor* UInventoryToPhysicalBridge::SpawnLogFromInventory(FName ItemID, const FVector& Location)
{
    return SpawnAfterInventoryCheck(this, ItemID, Location, LogClass);
}

AStoneActor* UInventoryToPhysicalBridge::SpawnStoneFromInventory(FName ItemID, const FVector& Location)
{
    return SpawnAfterInventoryCheck(this, ItemID, Location, StoneClass);
}

AStickActor* UInventoryToPhysicalBridge::SpawnStickFromInventory(FName ItemID, const FVector& Location)
{
    return SpawnAfterInventoryCheck(this, ItemID, Location, StickClass);
}
