#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Components/BoxComponent.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneDefinition.h"
#include "StonePhaseB_Placement/StonePlacementComponent.h"
#include "StonePhaseB_Placement/StoneCarryComponent.h"

namespace
{
    UWorld* CreateStonePlacementTestWorld()
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        if (!World)
        {
            return nullptr;
        }

        UWorld::InitializationValues Values;
        Values.AllowAudioPlayback(false);
        Values.RequiresHitProxies(false);
        Values.CreatePhysicsScene(true);
        Values.CreateNavigation(false);
        Values.CreateAISystem(false);
        Values.ShouldSimulatePhysics(false);
        World->InitializeNewWorld(Values);
        return World;
    }

    void DestroyStonePlacementTestWorld(UWorld* World)
    {
        if (World)
        {
            World->DestroyWorld(false);
        }
    }

    bool CreatePlacementFixture(
        UWorld* World,
        AActor*& OutOwner,
        AStoneActor*& OutStone,
        UStonePlacementComponent*& OutPlacement)
    {
        if (!World)
        {
            return false;
        }

        // ACharacter provides a root capsule and mesh component; tests cover the no-skeletal-socket fallback.
        OutOwner = World->SpawnActor<ACharacter>(FVector::ZeroVector, FRotator::ZeroRotator);
        OutStone = World->SpawnActor<AStoneActor>(
            FVector(17.0f, 23.0f, 41.0f), FRotator::ZeroRotator);
        if (!OutOwner || !OutStone)
        {
            return false;
        }

        UStoneDefinition* Definition = NewObject<UStoneDefinition>(OutStone);
        if (!Definition)
        {
            return false;
        }

        Definition->StoneName = FName(TEXT("AutomationStone"));
        Definition->Dimensions = FVector(30.0f, 20.0f, 20.0f);
        Definition->Mesh = TSoftObjectPtr<UStaticMesh>(
            FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")));

        FStoneFace BottomFace;
        BottomFace.FaceName = FName(TEXT("Bottom"));
        BottomFace.LocalNormal = FVector::DownVector;
        Definition->Faces.Add(BottomFace);

        FStoneFace SideFace;
        SideFace.FaceName = FName(TEXT("Side"));
        SideFace.LocalNormal = FVector::RightVector;
        Definition->Faces.Add(SideFace);

        FStoneSocket TopSocket;
        TopSocket.SocketName = FName(TEXT("TopSocket"));
        TopSocket.LocalPosition = FVector(10.0f, 0.0f, 0.0f);
        TopSocket.LocalRotation = FRotator::ZeroRotator;
        TopSocket.SocketRadius = 12.0f;
        Definition->Sockets.Add(TopSocket);

        OutStone->InitializeFromDefinition(Definition);

        OutPlacement = NewObject<UStonePlacementComponent>(OutOwner);
        if (!OutPlacement)
        {
            return false;
        }
        OutOwner->AddInstanceComponent(OutPlacement);
        OutPlacement->RegisterComponent();
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStonePlacementBeginCancelTest,
    "MyProject2.StonePlacement.Lifecycle.BeginCancel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStonePlacementBeginCancelTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateStonePlacementTestWorld();
    if (!TestNotNull(TEXT("Test world created"), World))
    {
        return false;
    }

    AActor* Owner = nullptr;
    AStoneActor* Stone = nullptr;
    UStonePlacementComponent* Placement = nullptr;
    const bool bFixtureValid = CreatePlacementFixture(World, Owner, Stone, Placement);
    TestTrue(TEXT("Placement fixture created"), bFixtureValid);
    if (!bFixtureValid)
    {
        DestroyStonePlacementTestWorld(World);
        return false;
    }

    const FTransform OriginalTransform = Stone->GetActorTransform();
    TestTrue(TEXT("Loose stone begins placement"), Placement->BeginPlacement(Stone));
    TestTrue(TEXT("Preview points to the source stone"), Placement->GetPreviewStone() == Stone);
    TestTrue(TEXT("Preview disables loose physics through carried state"),
        Stone->GetStructuralState_Implementation() == EStructuralNodeState::Carried);
    TestTrue(TEXT("Cancel restores the original physical object"), Placement->CancelPlacement());
    TestTrue(TEXT("Cancel clears the preview"), Placement->GetPreviewStone() == nullptr);
    TestTrue(TEXT("Cancel restores the source state"),
        Stone->GetStructuralState_Implementation() == EStructuralNodeState::Loose);
    TestTrue(TEXT("Cancel restores the original position"),
        Stone->GetActorLocation().Equals(OriginalTransform.GetLocation(), 0.01f));

    DestroyStonePlacementTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStonePlacementFaceSocketCycleTest,
    "MyProject2.StonePlacement.Selection.FaceAndSocket",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStonePlacementFaceSocketCycleTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateStonePlacementTestWorld();
    if (!TestNotNull(TEXT("Test world created"), World))
    {
        return false;
    }

    AActor* Owner = nullptr;
    AStoneActor* Stone = nullptr;
    UStonePlacementComponent* Placement = nullptr;
    const bool bFixtureValid = CreatePlacementFixture(World, Owner, Stone, Placement);
    TestTrue(TEXT("Placement fixture created"), bFixtureValid);
    if (!bFixtureValid)
    {
        DestroyStonePlacementTestWorld(World);
        return false;
    }

    const FVector ContactPoint(100.0f, 20.0f, 50.0f);
    TestTrue(TEXT("Placement begins"), Placement->BeginPlacement(Stone));
    TestTrue(TEXT("Surface target accepted"),
        Placement->UpdatePlacementTarget(ContactPoint, FVector::UpVector));
    TestEqual(TEXT("Default orientation does not preselect a face"),
        Placement->GetPlacementOptionIndex(), INDEX_NONE);

    Placement->CycleFace();
    TestEqual(TEXT("First cycle selects first valid face"), Placement->GetPlacementOptionIndex(), 0);
    FTransform Target = Placement->GetTargetTransform();
    FVector WorldNormal = Target.TransformVectorNoScale(FVector::DownVector).GetSafeNormal();
    TestTrue(TEXT("Selected bottom face aligns against the surface"),
        WorldNormal.Equals(-FVector::UpVector, 0.01f));

    Placement->CycleFace();
    TestEqual(TEXT("Second cycle selects the side face"), Placement->GetPlacementOptionIndex(), 1);
    Target = Placement->GetTargetTransform();
    WorldNormal = Target.TransformVectorNoScale(FVector::RightVector).GetSafeNormal();
    TestTrue(TEXT("Selected side face aligns against the surface"),
        WorldNormal.Equals(-FVector::UpVector, 0.01f));

    Placement->CycleFace();
    TestEqual(TEXT("Third cycle selects the available socket"), Placement->GetPlacementOptionIndex(), 2);
    Target = Placement->GetTargetTransform();
    const FVector SocketWorldPosition = Target.TransformPosition(FVector(10.0f, 0.0f, 0.0f));
    TestTrue(TEXT("Selected socket is snapped to the target surface point"),
        SocketWorldPosition.Equals(ContactPoint, 0.01f));

    Placement->CancelPlacement();
    DestroyStonePlacementTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStonePlacementConfirmGateTest,
    "MyProject2.StonePlacement.Lifecycle.ConfirmGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStonePlacementConfirmGateTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateStonePlacementTestWorld();
    if (!TestNotNull(TEXT("Test world created"), World))
    {
        return false;
    }

    AActor* Owner = nullptr;
    AStoneActor* Stone = nullptr;
    UStonePlacementComponent* Placement = nullptr;
    const bool bFixtureValid = CreatePlacementFixture(World, Owner, Stone, Placement);
    TestTrue(TEXT("Placement fixture created"), bFixtureValid);
    if (!bFixtureValid)
    {
        DestroyStonePlacementTestWorld(World);
        return false;
    }

    UStoneCarryComponent* Carry = NewObject<UStoneCarryComponent>(Owner);
    TestNotNull(TEXT("Carry component created"), Carry);
    if (!Carry)
    {
        DestroyStonePlacementTestWorld(World);
        return false;
    }
    Owner->AddInstanceComponent(Carry);
    Carry->RegisterComponent();
    TestTrue(TEXT("Stone enters the carry owner"), Carry->PickUp(Stone, EStoneCarryMode::TwoHands));
    TestTrue(TEXT("Carry owner references the same physical stone"), Carry->GetCarried() == Stone);

    TestTrue(TEXT("Placement begins from the carried stone"), Placement->BeginPlacement(Carry->GetCarried()));
    TestFalse(TEXT("Placement cannot commit before a valid target"),
        Placement->ConfirmPlacement());
    TestTrue(TEXT("Rejected commit preserves carried state"),
        Stone->GetStructuralState_Implementation() == EStructuralNodeState::Carried);
    TestFalse(TEXT("A zero surface normal is rejected"),
        Placement->UpdatePlacementTarget(FVector(80.0f, 0.0f, 25.0f), FVector::ZeroVector));
    TestFalse(TEXT("Invalid target still cannot be committed"), Placement->ConfirmPlacement());
    TestTrue(TEXT("Cancel restores carried state"), Placement->CancelPlacement());
    TestTrue(TEXT("Cancel retains carry ownership"), Carry->GetCarried() == Stone);
    TestTrue(TEXT("Cancelled stone remains carried"),
        Stone->GetStructuralState_Implementation() == EStructuralNodeState::Carried);

    TestTrue(TEXT("Placement can be started again"), Placement->BeginPlacement(Carry->GetCarried()));

    AActor* SupportSurface = World->SpawnActor<AActor>(
        FVector(80.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
    TestNotNull(TEXT("Support surface actor created"), SupportSurface);
    if (!SupportSurface)
    {
        DestroyStonePlacementTestWorld(World);
        return false;
    }
    UBoxComponent* SupportBox = NewObject<UBoxComponent>(SupportSurface, TEXT("SupportSurface"));
    TestNotNull(TEXT("Support surface collision created"), SupportBox);
    if (!SupportBox)
    {
        DestroyStonePlacementTestWorld(World);
        return false;
    }
    SupportSurface->SetRootComponent(SupportBox);
    SupportBox->SetBoxExtent(FVector(300.0f, 300.0f, 50.0f));
    SupportBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SupportBox->SetCollisionObjectType(ECC_WorldStatic);
    SupportBox->SetCollisionResponseToAllChannels(ECR_Block);
    SupportBox->RegisterComponent();

    TestTrue(TEXT("A valid support surface can be targeted"),
        Placement->UpdatePlacementTarget(FVector(80.0f, 0.0f, 50.0f), FVector::UpVector, SupportSurface));
    TestTrue(TEXT("Placement commits when the only overlap is the support surface"), Placement->ConfirmPlacement());
    TestTrue(TEXT("Preview is cleared after success"), Placement->GetPreviewStone() == nullptr);
    TestTrue(TEXT("Committed stone is installed"),
        Stone->GetStructuralState_Implementation() == EStructuralNodeState::Installed);
    TestFalse(TEXT("Carry owner releases the stone after placement"), Carry->IsCarrying());
    TestTrue(TEXT("Carry owner no longer owns the installed stone"), Carry->GetCarried() == nullptr);

    DestroyStonePlacementTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStonePlacementCollisionClearanceTest,
    "MyProject2.StonePlacement.Validation.CollisionClearance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStonePlacementCollisionClearanceTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateStonePlacementTestWorld();
    if (!TestNotNull(TEXT("Test world created"), World))
    {
        return false;
    }

    AActor* Owner = nullptr;
    AStoneActor* Stone = nullptr;
    UStonePlacementComponent* Placement = nullptr;
    const bool bFixtureValid = CreatePlacementFixture(World, Owner, Stone, Placement);
    TestTrue(TEXT("Placement fixture created"), bFixtureValid);
    if (!bFixtureValid)
    {
        DestroyStonePlacementTestWorld(World);
        return false;
    }

    AActor* Blocker = World->SpawnActor<AActor>(
        FVector(200.0f, 0.0f, 50.0f), FRotator::ZeroRotator);
    TestNotNull(TEXT("Blocking actor created"), Blocker);
    if (!Blocker)
    {
        DestroyStonePlacementTestWorld(World);
        return false;
    }

    UBoxComponent* BlockerBox = NewObject<UBoxComponent>(Blocker, TEXT("PlacementBlocker"));
    TestNotNull(TEXT("Blocking collision component created"), BlockerBox);
    if (!BlockerBox)
    {
        DestroyStonePlacementTestWorld(World);
        return false;
    }

    Blocker->SetRootComponent(BlockerBox);
    BlockerBox->SetBoxExtent(FVector(20.0f, 20.0f, 20.0f));
    BlockerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BlockerBox->SetCollisionObjectType(ECC_WorldStatic);
    BlockerBox->SetCollisionResponseToAllChannels(ECR_Block);
    BlockerBox->RegisterComponent();

    TestTrue(TEXT("Placement begins"), Placement->BeginPlacement(Stone));
    TestTrue(TEXT("Surface target accepted"),
        Placement->UpdatePlacementTarget(FVector(200.0f, 0.0f, 0.0f), FVector::UpVector));
    TestFalse(TEXT("Placement refuses a physical overlap"),
        Placement->ConfirmPlacement());
    TestTrue(TEXT("Blocked placement keeps preview active"),
        Placement->GetPreviewStone() == Stone);
    TestTrue(TEXT("Blocked placement preserves carried state"),
        Stone->GetStructuralState_Implementation() == EStructuralNodeState::Carried);

    Placement->CancelPlacement();
    DestroyStonePlacementTestWorld(World);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
