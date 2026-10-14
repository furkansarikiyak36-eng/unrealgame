#include "BarnActor.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "HerdManager.h"
ABarnActor::ABarnActor(){PrimaryActorTick.bCanEverTick=false;BarnMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Barn"));RootComponent=BarnMesh;}
void ABarnActor::BeginPlay(){Super::BeginPlay();if(GetWorld())if(UHerdManager* H=GetWorld()->GetSubsystem<UHerdManager>())H->RegisterBarn(this);}
void ABarnActor::EndPlay(const EEndPlayReason::Type R){Super::EndPlay(R);}
bool ABarnActor::TryAddAnimal(){if(Occupancy>=FMath::Max(0,Capacity))return false;++Occupancy;return true;}
void ABarnActor::RemoveAnimal(){Occupancy=FMath::Max(0,Occupancy-1);}
