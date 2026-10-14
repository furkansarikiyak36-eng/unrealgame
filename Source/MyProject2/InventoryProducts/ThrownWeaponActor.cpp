#include "ThrownWeaponActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
AThrownWeaponActor::AThrownWeaponActor(){PrimaryActorTick.bCanEverTick=true;Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));RootComponent=Mesh;Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));Mesh->SetSimulatePhysics(false);ExplosionSphere=CreateDefaultSubobject<USphereComponent>(TEXT("Explosion"));ExplosionSphere->SetupAttachment(Mesh);ExplosionSphere->SetSphereRadius(ExplosionRadius);ExplosionFX=CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FX"));ExplosionFX->SetupAttachment(Mesh);ExplosionFX->SetAutoActivate(false);}
void AThrownWeaponActor::Throw(const FVector&D,float F){if(UPrimitiveComponent*P=Cast<UPrimitiveComponent>(RootComponent)){P->SetSimulatePhysics(true);P->AddImpulse(D.GetSafeNormal()*F);}ElapsedTime=0;bDetonated=false;}
void AThrownWeaponActor::SetFuseTime(float S){FuseTime=FMath::Max(0.0f,S);ElapsedTime=0;bDetonated=false;}
void AThrownWeaponActor::Detonate(){if(bDetonated)return;bDetonated=true;if(ExplosionFX)ExplosionFX->Activate(true);for(TActorIterator<AActor>It(GetWorld());It;++It){AActor*A=*It;if(!A||A==this)continue;const float D=FVector::Dist(GetActorLocation(),A->GetActorLocation());if(D<=ExplosionRadius){const float Falloff=1.0f-D/FMath::Max(1.0f,ExplosionRadius);/* Apply Damage through the project's damage interface; type-specific effects remain game-side. */(void)Falloff;}}SetLifeSpan(2.0f);}
void AThrownWeaponActor::Tick(float D){Super::Tick(D);if(bDetonated)return;ElapsedTime+=D;if(ElapsedTime>=FuseTime)Detonate();}
