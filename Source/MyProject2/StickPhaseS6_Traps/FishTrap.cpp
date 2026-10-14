#include "FishTrap.h"
#include "Components/StaticMeshComponent.h"
#include "StickPhaseS6_Traps/TrapMechanismComponent.h"

AFishTrap::AFishTrap(){PrimaryActorTick.bCanEverTick = true;
 TrapMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrapMesh")); RootComponent=TrapMesh; Mechanism=CreateDefaultSubobject<UTrapMechanismComponent>(TEXT("Mechanism"));}
#include "StickPhaseS6_Traps/TrapMechanismComponent.h"
void AFishTrap::PlaceInWater(const FVector&L){SetActorLocation(L);} int32 AFishTrap::CollectFish(){int32 N=StoredFish;StoredFish=0;return N;} void AFishTrap::Tick(float D){Super::Tick(D);if(StoredFish<MaxFish&&FMath::FRand()<CatchRate*D)++StoredFish;}
