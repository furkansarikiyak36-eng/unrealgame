#include "StoneStoragePile.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "Components/StaticMeshComponent.h"
AStoneStoragePile::AStoneStoragePile()
{
    PrimaryActorTick.bCanEverTick = false;
    PileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pile")); RootComponent = PileMesh;
}
bool AStoneStoragePile::Store(AStoneActor* Stone)
{
    if (!Stone || Stored.Num() >= Capacity || !Stone->SetStructuralState_Implementation(EStructuralNodeState::Carried)) return false;
    Stone->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform); Stored.Add(Stone); return true;
}
AStoneActor* AStoneStoragePile::Retrieve()
{
    if (Stored.Num() == 0) return nullptr;
    AStoneActor* Stone = Stored.Pop().Get();
    if (Stone) { Stone->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform); Stone->SetStructuralState_Implementation(EStructuralNodeState::Loose); }
    return Stone;
}
