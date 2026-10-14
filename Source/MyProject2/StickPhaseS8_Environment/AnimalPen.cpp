#include "AnimalPen.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"

AAnimalPen::AAnimalPen(){PrimaryActorTick.bCanEverTick = false;
 PenMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PenMesh")); RootComponent=PenMesh; CaptureZone=CreateDefaultSubobject<UBoxComponent>(TEXT("CaptureZone")); CaptureZone->SetupAttachment(RootComponent);}
bool AAnimalPen::CaptureAnimal(AActor*A){if(!A||CapturedAnimals.Num()>=MaxAnimals)return false;CapturedAnimals.Add(A);return true;} AActor* AAnimalPen::ReleaseAnimal(int32 I){if(!CapturedAnimals.IsValidIndex(I))return nullptr;AActor*A=CapturedAnimals[I];CapturedAnimals.RemoveAt(I);return A;}
