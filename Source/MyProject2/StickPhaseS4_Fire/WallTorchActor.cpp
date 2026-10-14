#include "WallTorchActor.h"
#include "Components/StaticMeshComponent.h"
#include "StickPhaseS4_Fire/TorchComponent.h"

AWallTorchActor::AWallTorchActor(){PrimaryActorTick.bCanEverTick = false;
 MountMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MountMesh")); RootComponent=MountMesh; TorchComponent=CreateDefaultSubobject<UTorchComponent>(TEXT("TorchComponent"));}
#include "StickPhaseS4_Fire/TorchComponent.h"
bool AWallTorchActor::AttachToWall(AActor* W){if(!W)return false;AttachToActor(W,FAttachmentTransformRules::KeepWorldTransform);AttachedWall=W;return true;}
