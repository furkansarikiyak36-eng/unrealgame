#include "TorchActor.h"
#include "Components/StaticMeshComponent.h"
#include "StickPhaseS4_Fire/TorchComponent.h"
#include "Components/PointLightComponent.h"

ATorchActor::ATorchActor(){PrimaryActorTick.bCanEverTick = false;
 TorchMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TorchMesh")); RootComponent=TorchMesh; TorchLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("TorchLight")); TorchLight->SetupAttachment(RootComponent); TorchComponent=CreateDefaultSubobject<UTorchComponent>(TEXT("TorchComponent"));}
#include "StickPhaseS4_Fire/TorchComponent.h"
void ATorchActor::Ignite(){if(TorchComponent)TorchComponent->Ignite(1.f);} void ATorchActor::Extinguish(){if(TorchComponent)TorchComponent->Extinguish();}
