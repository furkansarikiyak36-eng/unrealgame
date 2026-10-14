#include "BowActor.h"
#include "Components/StaticMeshComponent.h"
#include "ArrowActor.h"

ABowActor::ABowActor(){PrimaryActorTick.bCanEverTick = false;
 BowMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BowMesh")); RootComponent=BowMesh;}
void ABowActor::DrawBow(float A){CurrentDraw=FMath::Clamp(CurrentDraw+A,0.f,1.f);}
void ABowActor::ReleaseBow(){if(!NockedArrow)return;NockedArrow->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);NockedArrow->LaunchArrow(GetActorForwardVector(),MaxDrawForce*CurrentDraw);NockedArrow=nullptr;CurrentDraw=0.f;}
void ABowActor::NockArrow(AArrowActor* A){if(A){A->AttachToComponent(BowMesh,FAttachmentTransformRules::SnapToTargetNotIncludingScale);NockedArrow=A;}}
