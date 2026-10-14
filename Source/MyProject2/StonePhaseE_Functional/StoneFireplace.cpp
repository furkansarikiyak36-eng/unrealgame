#include "StoneFireplace.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Particles/ParticleSystemComponent.h"
AStoneFireplace::AStoneFireplace()
{
    PrimaryActorTick.bCanEverTick = true;
    FireplaceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fireplace")); RootComponent = FireplaceMesh;
    FireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight")); FireLight->SetupAttachment(FireplaceMesh); FireLight->SetIntensity(0.0f);
    FireParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FireParticles")); FireParticles->SetupAttachment(FireplaceMesh); FireParticles->SetActive(false);
}
bool AStoneFireplace::AddFuel(float Amount)
{
    if (Amount <= 0.0f || MaxFuel <= 0.0f || CurrentFuel >= MaxFuel) return false;
    CurrentFuel = FMath::Min(CurrentFuel + Amount, MaxFuel); return true;
}
bool AStoneFireplace::Ignite()
{
    if (CurrentFuel <= 0.0f) return false;
    bIsLit = true; FireLight->SetIntensity(3000.0f); FireParticles->SetActive(true); return true;
}
void AStoneFireplace::Extinguish()
{
    bIsLit = false; FireLight->SetIntensity(0.0f); FireParticles->SetActive(false);
}
bool AStoneFireplace::PlaceCookingPot()
{
    if (bHasCookingPot || !bIsLit) return false;
    bHasCookingPot = true; return true;
}
void AStoneFireplace::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!bIsLit) return;
    CurrentFuel = FMath::Max(0.0f, CurrentFuel - DeltaTime * 2.0f);
    if (CurrentFuel <= 0.0f) { Extinguish(); return; }
    const float Alpha = MaxFuel > 0.0f ? CurrentFuel / MaxFuel : 0.0f;
    FireLight->SetIntensity(FMath::Max(0.0f, FMath::Lerp(0.0f, 3000.0f, Alpha) + FMath::RandRange(-200.f, 200.f)));
}
