#include "StandingFireActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Particles/ParticleSystemComponent.h"

AStandingFireActor::AStandingFireActor(){PrimaryActorTick.bCanEverTick = true;
 FireMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FireMesh")); RootComponent=FireMesh; FireLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight")); FireLight->SetupAttachment(RootComponent); FireParticles=CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FireParticles")); FireParticles->SetupAttachment(RootComponent);}
bool AStandingFireActor::Ignite(){if(Fuel<=0)return false;bIsLit=true;FireLight->SetIntensity(2500);FireParticles->SetActive(true);return true;} void AStandingFireActor::Extinguish(){bIsLit=false;FireLight->SetIntensity(0);FireParticles->SetActive(false);} bool AStandingFireActor::AddFuel(float A){if(A<=0||Fuel>=MaxFuel)return false;Fuel=FMath::Min(Fuel+A,MaxFuel);return true;} void AStandingFireActor::Tick(float D){Super::Tick(D);if(!bIsLit)return;Fuel=FMath::Max(0.f,Fuel-.8f*D);if(Fuel<=0)Extinguish();}
