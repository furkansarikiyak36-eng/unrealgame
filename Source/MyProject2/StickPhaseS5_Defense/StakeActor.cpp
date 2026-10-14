#include "StakeActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

AStakeActor::AStakeActor(){PrimaryActorTick.bCanEverTick = false;
 DamageSphere=CreateDefaultSubobject<USphereComponent>(TEXT("DamageSphere")); DamageSphere->SetupAttachment(RootComponent); StakeMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StakeMesh")); RootComponent=StakeMesh;}
void AStakeActor::Sharpen(){if(!bSharpened){bSharpened=true;Damage*=1.5f;}} void AStakeActor::ApplyDamageToTarget(AActor* T,float D){if(T&&bSharpened){/* Project damage interface hook. */}}
