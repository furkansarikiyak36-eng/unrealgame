#include "WaterCollector.h"
#include "Components/StaticMeshComponent.h"

AWaterCollector::AWaterCollector(){PrimaryActorTick.bCanEverTick = true;
 FrameMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh")); RootComponent=FrameMesh;}
float AWaterCollector::CollectWater(float A){if(!bHasContainer||A<=0)return 0;float N=FMath::Min(A,CurrentWater);CurrentWater-=N;return N;} bool AWaterCollector::AddContainer(){if(bHasContainer)return false;bHasContainer=true;return true;} void AWaterCollector::Tick(float D){Super::Tick(D);/* Replace with weather-system precipitation input. */}
