#include "WorldItemActor.h"
#include "InventoryCore/InventoryItem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
AWorldItemActor::AWorldItemActor(){PrimaryActorTick.bCanEverTick=false;MeshComponent=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));RootComponent=MeshComponent;MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));MeshComponent->SetSimulatePhysics(true);InteractionSphere=CreateDefaultSubobject<USphereComponent>(TEXT("Interact"));InteractionSphere->SetupAttachment(MeshComponent);InteractionSphere->SetSphereRadius(InteractRadius);InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));}
void AWorldItemActor::InitializeFromItemData(UInventoryItemData* D,int32 Count){if(!D||Count<=0)return;ItemData=D->ToInventoryItem(Count);InteractRadius=FMath::Max(1.0f,InteractRadius);InteractionSphere->SetSphereRadius(InteractRadius);if(UStaticMesh*M=D->WorldMesh.LoadSynchronous())MeshComponent->SetStaticMesh(M);}
