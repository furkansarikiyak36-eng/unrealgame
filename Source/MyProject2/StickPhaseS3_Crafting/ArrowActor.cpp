#include "ArrowActor.h"
#include "Components/StaticMeshComponent.h"

AArrowActor::AArrowActor(){PrimaryActorTick.bCanEverTick = false;
 ArrowMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowMesh")); RootComponent=ArrowMesh;}
void AArrowActor::LaunchArrow(const FVector&D,float F){if(ArrowMesh){ArrowMesh->SetSimulatePhysics(true);ArrowMesh->AddImpulse(D.GetSafeNormal()*F);}}
void AArrowActor::SetStoneTip(bool B){bStoneTip=B;BaseDamage=B?30.f:20.f;}
