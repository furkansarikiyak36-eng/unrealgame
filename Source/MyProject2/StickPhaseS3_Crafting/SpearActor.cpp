#include "SpearActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

ASpearActor::ASpearActor(){PrimaryActorTick.bCanEverTick = false;
 SpearMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpearMesh")); RootComponent=SpearMesh; DamageSphere=CreateDefaultSubobject<USphereComponent>(TEXT("DamageSphere")); DamageSphere->SetupAttachment(RootComponent);}
void ASpearActor::ThrowSpear(const FVector& D,float F){bThrown=true;if(SpearMesh){SpearMesh->SetSimulatePhysics(true);SpearMesh->AddImpulse(D.GetSafeNormal()*F);}}
void ASpearActor::MeleeAttack(AActor* Target,float DamageAmount){if(Target){/* Project damage interface hook. */}}
