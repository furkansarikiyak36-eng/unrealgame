#include "MilkingStation.h"
#include "PhaseT6_Animal/AnimalActor.h"
#include "MilkProductionComponent.h"
#include "Components/StaticMeshComponent.h"
AMilkingStation::AMilkingStation(){PrimaryActorTick.bCanEverTick=false;StationMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Station"));RootComponent=StationMesh;}
bool AMilkingStation::MilkAnimal(AAnimalActor* A){if(!A)return false;UMilkProductionComponent* M=A->FindComponentByClass<UMilkProductionComponent>();if(!M)return false;const float Space=FMath::Max(0.f,MilkStorageMax-MilkStorage);const float Amount=M->Milking(Space);MilkStorage+=Amount;return Amount>0.f;}
