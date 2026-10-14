#include "StickBed.h"
#include "Components/StaticMeshComponent.h"

AStickBed::AStickBed(){PrimaryActorTick.bCanEverTick = false;
 BedMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BedMesh")); RootComponent=BedMesh;}
bool AStickBed::Sleep(AActor*S){if(!S||bOccupied)return false;bOccupied=true;return true;} void AStickBed::WakeUp(){bOccupied=false;}
