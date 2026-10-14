#include "StickChair.h"
#include "Components/StaticMeshComponent.h"

AStickChair::AStickChair(){PrimaryActorTick.bCanEverTick = false;
 ChairMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChairMesh")); RootComponent=ChairMesh;}
bool AStickChair::Sit(AActor*S){if(!S||bOccupied)return false;bOccupied=true;return true;} void AStickChair::Stand(){bOccupied=false;}
