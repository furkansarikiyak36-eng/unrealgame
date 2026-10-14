#include "Scarecrow.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

AScarecrow::AScarecrow(){PrimaryActorTick.bCanEverTick = false;
 ScarecrowMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScarecrowMesh")); RootComponent=ScarecrowMesh; ScareVolume=CreateDefaultSubobject<USphereComponent>(TEXT("ScareVolume")); ScareVolume->SetupAttachment(RootComponent);}
void AScarecrow::SetScareRadius(float R){ScareRadius=FMath::Max(0.f,R);ScareVolume->SetSphereRadius(ScareRadius);}
