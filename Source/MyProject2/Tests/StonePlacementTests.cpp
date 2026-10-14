#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneDefinition.h"
#include "StonePhaseB_Placement/StonePlacementComponent.h"

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
        Values.CreatePhysicsScene(false);
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

        OutOwner = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
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
    TestEqual(TEXT("Preview disables loose physics through carried state"),
        Stone->GetStructuralState_Implementation(), EStructuralNodeState::Carried);
    TestTrue(TEXT("Cancel restores the original physical object"), Placement->CancelPlacement());
    TestTrue(TEXT("Cancel clears the preview"), Placement->GetPreviewStone() == nullptr);
    TestEqual(TEXT("Cancel restores the source state"),
        Stone->GetStructuralState_Implementation(), EStructuralNodeState::Loose);
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

    TestTrue(TEXT("Placement begins"), Placement->BeginPlacement(Stone));
    TestFalse(TEXT("Placement cannot commit before a valid target"),
        Placement->ConfirmPlacement());
    TestEqual(TEXT("Rejected commit preserves carried state"),
        Stone->GetStructuralState_Implementation(), EStructuralNodeState::Carried);

    TestTrue(TEXT("A valid target can be set"),
        Placement->UpdatePlacementTarget(FVector(80.0f, 0.0f, 25.0f), FVector::UpVector));
    TestTrue(TEXT("Placement commits after a valid target"), Placement->ConfirmPlacement());
    TestTrue(TEXT("Preview is cleared after success"), Placement->GetPreviewStone() == nullptr);
    TestEqual(TEXT("Committed stone is installed"),
        Stone->GetStructuralState_Implementation(), EStructuralNodeState::Installed);

    DestroyStonePlacementTestWorld(World);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
