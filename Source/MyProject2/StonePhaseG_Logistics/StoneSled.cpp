#include "StoneSled.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
AStoneSled::AStoneSled()
{
    PrimaryActorTick.bCanEverTick = false;
    Platform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Platform")); RootComponent = Platform;
    Platform->SetSimulatePhysics(true);
}
bool AStoneSled::LoadStone(AStoneActor* Stone)
{
    if (!Stone || Loaded.Num() >= Capacity || !Stone->SetStructuralState_Implementation(EStructuralNodeState::Carried)) return false;
    Stone->AttachToComponent(Platform, FAttachmentTransformRules::KeepWorldTransform); Loaded.Add(Stone); return true;
}
void AStoneSled::SetDragForce(float Force)
{
    if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(RootComponent)) P->AddForce(GetActorForwardVector() * Force);
}
