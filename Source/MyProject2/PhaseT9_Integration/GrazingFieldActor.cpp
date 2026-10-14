#include "GrazingFieldActor.h"
#include "Components/StaticMeshComponent.h"
AGrazingFieldActor::AGrazingFieldActor(){PrimaryActorTick.bCanEverTick=false;FieldMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Field"));RootComponent=FieldMesh;}
void AGrazingFieldActor::UpdateDaily(float Growth,float Consumption){GrassBiomass=FMath::Clamp(GrassBiomass+FMath::Max(0.f,Growth)-FMath::Max(0.f,Consumption),0.f,FMath::Max(0.f,MaxGrass));}
float AGrazingFieldActor::ConsumeGrass(float A){const float T=FMath::Min(FMath::Max(0.f,A),GrassBiomass);GrassBiomass-=T;return T;}
