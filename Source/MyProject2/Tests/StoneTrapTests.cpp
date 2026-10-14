#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "RaidEnemy.h"
#include "StonePhaseF_Environment/StoneTrapActor.h"

namespace
{
    UWorld* CreateStoneTrapTestWorld()
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        if (!World)
        {
            return nullptr;
        }

        UWorld::InitializationValues Values;
        Values.AllowAudioPlayback(false);
        Values.RequiresHitProxies(false);
        Values.CreatePhysicsScene(false);
        Values.CreateNavigation(false);
        Values.CreateAISystem(false);
        Values.ShouldSimulatePhysics(false);
        World->InitializeNewWorld(Values);
        return World;
    }

    void DestroyStoneTrapTestWorld(UWorld* World)
    {
        if (World)
        {
            World->DestroyWorld(false);
        }
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStoneTrapDamageAndCooldownTest,
    "MyProject2.StoneTrap.Damage.DispatchAndCooldown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoneTrapDamageAndCooldownTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateStoneTrapTestWorld();
    if (!TestNotNull(TEXT("Test world created"), World))
    {
        return false;
    }

    AStoneTrapActor* Trap = World->SpawnActor<AStoneTrapActor>(
        FVector::ZeroVector, FRotator::ZeroRotator);
    ARaidEnemy* Enemy = World->SpawnActor<ARaidEnemy>(
        FVector(100.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
    TestNotNull(TEXT("Stone trap created"), Trap);
    TestNotNull(TEXT("Raid enemy created"), Enemy);
    if (!Trap || !Enemy)
    {
        DestroyStoneTrapTestWorld(World);
        return false;
    }

    Trap->TrapType = EStoneTrapType::SpikeStone;
    Trap->Damage = 10.0f;
    Trap->Cooldown = 8.0f;

    TestTrue(TEXT("Trigger volume generates overlap events"), Trap->TriggerVolume->GetGenerateOverlapEvents());
    TestTrue(TEXT("Trigger volume is query-only"),
        Trap->TriggerVolume->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
    TestTrue(TEXT("Pawn overlaps activate the trigger"),
        Trap->TriggerVolume->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap);
    TestTrue(TEXT("Physical stone/log bodies activate the trigger"),
        Trap->TriggerVolume->GetCollisionResponseToChannel(ECC_PhysicsBody) == ECR_Overlap);

    Trap->Trigger(Enemy);
    TestTrue(TEXT("First trigger applies configured damage"),
        FMath::IsNearlyEqual(Enemy->Health, 50.0f, 0.01f));
    TestFalse(TEXT("Trap disarms after the first trigger"), Trap->bArmed);

    Trap->Rearm();
    TestFalse(TEXT("Rearm is rejected while cooldown is active"), Trap->bArmed);
    Trap->Trigger(Enemy);
    TestTrue(TEXT("Cooldown prevents immediate repeated damage"),
        FMath::IsNearlyEqual(Enemy->Health, 50.0f, 0.01f));

    Trap->Cooldown = 0.0f;
    Trap->Rearm();
    Trap->Trigger(Enemy);
    TestTrue(TEXT("Damage can be applied again after cooldown allows it"),
        FMath::IsNearlyEqual(Enemy->Health, 40.0f, 0.01f));

    DestroyStoneTrapTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStoneTrapNullVictimSafetyTest,
    "MyProject2.StoneTrap.Damage.NullVictimSafety",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoneTrapNullVictimSafetyTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateStoneTrapTestWorld();
    if (!TestNotNull(TEXT("Test world created"), World))
    {
        return false;
    }

    AStoneTrapActor* Trap = World->SpawnActor<AStoneTrapActor>(
        FVector::ZeroVector, FRotator::ZeroRotator);
    TestNotNull(TEXT("Stone trap created"), Trap);
    if (!Trap)
    {
        DestroyStoneTrapTestWorld(World);
        return false;
    }

    Trap->TrapType = EStoneTrapType::SpikeStone;
    Trap->Damage = 15.0f;
    Trap->Cooldown = 8.0f;
    Trap->Trigger(nullptr);
    TestFalse(TEXT("Trap safely disarms without a victim"), Trap->bArmed);

    Trap->Rearm();
    TestFalse(TEXT("Rearm remains blocked before cooldown expires"), Trap->bArmed);
    Trap->Trigger(nullptr);
    TestFalse(TEXT("Null victim remains safe and trap stays disarmed"), Trap->bArmed);

    DestroyStoneTrapTestWorld(World);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
