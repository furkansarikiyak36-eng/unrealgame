#include "StoneTrapActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
AStoneTrapActor::AStoneTrapActor()
{
    PrimaryActorTick.bCanEverTick = false;
    TrapMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Trap")); RootComponent = TrapMesh;
    TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger")); TriggerVolume->SetupAttachment(TrapMesh);
    TriggerVolume->SetBoxExtent(FVector(150, 150, 100));
}
void AStoneTrapActor::Trigger(AActor* Victim)
{
    if (!bArmed || !GetWorld()) return;
    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastTriggerTime < Cooldown) return;
    LastTriggerTime = Now; bArmed = false;
    if (TrapType == EStoneTrapType::RollingBoulder)
    {
        TrapMesh->SetMobility(EComponentMobility::Movable); TrapMesh->SetSimulatePhysics(true);
        TrapMesh->AddImpulse(GetActorForwardVector() * 5000.0f, NAME_None, true);
    }
    else if (TrapType == EStoneTrapType::FallingStone)
    {
        TrapMesh->SetMobility(EComponentMobility::Movable); TrapMesh->SetSimulatePhysics(true);
    }
    // TODO: route Damage to the game's damage interface/component. Victim is deliberately not assumed to support it.
    (void)Victim;
}
void AStoneTrapActor::Rearm() { bArmed = true; }
