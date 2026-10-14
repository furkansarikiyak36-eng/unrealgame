#include "StoneTrapActor.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "RaidEnemy.h"
#include "ForestCharacter.h"
#include "CarvableLog.h"
#include "StonePhaseA_Mining/StoneActor.h"
AStoneTrapActor::AStoneTrapActor()
{
    PrimaryActorTick.bCanEverTick = false;
    TrapMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Trap"));
    RootComponent = TrapMesh;

    TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
    TriggerVolume->SetupAttachment(TrapMesh);
    TriggerVolume->SetBoxExtent(FVector(150.0f, 150.0f, 100.0f));
    TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerVolume->SetCollisionObjectType(ECC_WorldDynamic);
    TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    TriggerVolume->SetGenerateOverlapEvents(true);
    TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AStoneTrapActor::HandleTriggerOverlap);
}

void AStoneTrapActor::HandleTriggerOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (IsValid(OtherActor) && OtherActor != this)
    {
        Trigger(OtherActor);
    }
}

void AStoneTrapActor::Trigger(AActor* Victim)
{
    if (!bArmed || !GetWorld() || !FMath::IsFinite(Cooldown) || Cooldown < 0.0f)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (LastTriggerTime >= 0.0f && Now - LastTriggerTime < Cooldown)
    {
        return;
    }

    LastTriggerTime = Now;
    bArmed = false;

    if (TrapType == EStoneTrapType::RollingBoulder)
    {
        TrapMesh->SetMobility(EComponentMobility::Movable);
        TrapMesh->SetSimulatePhysics(true);
        TrapMesh->AddImpulse(GetActorForwardVector() * 5000.0f, NAME_None, true);
    }
    else if (TrapType == EStoneTrapType::FallingStone)
    {
        TrapMesh->SetMobility(EComponentMobility::Movable);
        TrapMesh->SetSimulatePhysics(true);
    }

    if (!IsValid(Victim) || Victim == this || !FMath::IsFinite(Damage) || Damage <= 0.0f)
    {
        return;
    }

    // Use the established receiver methods where they exist; other gameplay actors can
    // implement Unreal's standard AnyDamage/TakeDamage path without a hard class dependency.
    if (ARaidEnemy* Enemy = Cast<ARaidEnemy>(Victim))
    {
        Enemy->TakeHit(Damage);
    }
    else if (AForestCharacter* Player = Cast<AForestCharacter>(Victim))
    {
        Player->TakeEnemyHit(Damage);
    }
    else if (AStoneActor* Stone = Cast<AStoneActor>(Victim))
    {
        Stone->ApplyDamage(Damage);
    }
    else if (ACarvableLog* Log = Cast<ACarvableLog>(Victim))
    {
        const float DamageFraction = FMath::Clamp(
            Damage * FMath::Max(0.0f, LogDamageFractionPerPoint), 0.0f, 1.0f);
        if (DamageFraction > 0.0f)
        {
            Log->AddDamage(DamageFraction);
        }
    }
    else
    {
        UGameplayStatics::ApplyDamage(
            Victim, Damage, nullptr, this, UDamageType::StaticClass());
    }
}

void AStoneTrapActor::Rearm()
{
    UWorld* World = GetWorld();
    if (!World || !FMath::IsFinite(Cooldown) || Cooldown < 0.0f)
    {
        bArmed = false;
        return;
    }

    const float Now = World->GetTimeSeconds();
    bArmed = LastTriggerTime < 0.0f || Now - LastTriggerTime >= Cooldown;
}
