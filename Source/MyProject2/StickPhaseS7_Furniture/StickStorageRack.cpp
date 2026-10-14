#include "StickStorageRack.h"
#include "Components/StaticMeshComponent.h"

AStickStorageRack::AStickStorageRack(){PrimaryActorTick.bCanEverTick = false;
 RackMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RackMesh")); RootComponent=RackMesh;}
#include "StickPhaseS1_Core/StickActor.h"
bool AStickStorageRack::Store(AStickActor*S){if(!S||Stored.Num()>=Capacity)return false;S->SetStructuralState_Implementation(EStructuralNodeState::Carried);S->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform);Stored.Add(S);return true;} AStickActor* AStickStorageRack::Retrieve(){if(Stored.IsEmpty())return nullptr;AStickActor*S=Stored.Pop();if(S){S->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);S->SetStructuralState_Implementation(EStructuralNodeState::Loose);}return S;}
