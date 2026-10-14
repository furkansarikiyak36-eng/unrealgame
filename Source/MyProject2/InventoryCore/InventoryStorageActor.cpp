#include "InventoryStorageActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "InventoryCore/InventoryComponent.h"

AInventoryStorageActor::AInventoryStorageActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StorageMesh"));
    RootComponent = Mesh;
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    Mesh->SetRelativeScale3D(FVector(1.5f, 0.7f, 1.0f));
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetSimulatePhysics(false);
    Mesh->SetMobility(EComponentMobility::Movable);

    Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("StorageInventory"));
    Inventory->MaxSlots = 80;
    Inventory->MaxWeight = 2000.0f;
    Inventory->bIgnoreWeight = false;
}

void AInventoryStorageActor::BeginPlay()
{
    Super::BeginPlay();
    if (StorageKey.IsNone())
    {
        StorageKey = GetFName();
    }
    if (Inventory)
    {
        Inventory->InitializeRuntimeInventory();
    }
}

bool AInventoryStorageActor::DepositFrom(UInventoryComponent* Source, FName ItemID, int32 Count)
{
    return Inventory && UInventoryComponent::TransferItem(Source, Inventory, ItemID, Count);
}

bool AInventoryStorageActor::WithdrawTo(UInventoryComponent* Destination, FName ItemID, int32 Count)
{
    return Inventory && UInventoryComponent::TransferItem(Inventory, Destination, ItemID, Count);
}
