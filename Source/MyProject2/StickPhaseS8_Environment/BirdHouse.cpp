#include "BirdHouse.h"
#include "Components/StaticMeshComponent.h"

ABirdHouse::ABirdHouse(){PrimaryActorTick.bCanEverTick = false;
 HouseMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HouseMesh")); RootComponent=HouseMesh;}
void ABirdHouse::AttachToTreeOrWall(AActor*H){if(H)AttachToActor(H,FAttachmentTransformRules::KeepWorldTransform);} void ABirdHouse::SpawnBird(){/* Bind to fauna spawner subsystem. */}
