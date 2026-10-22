// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Shoring/ExcavationShoringSubsystem.h"
#include "Shoring/PlaceableShoringPiece.h"
#include "Shoring/ShoringLayoutPlanner.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"

namespace
{
    UWorld* CreateShoringTestWorld(const TCHAR* Name)
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, FName(Name));
        if (!World)
        {
            return nullptr;
        }

        World->InitializeNewWorld(
            UWorld::InitializationValues()
                .AllowAudioPlayback(false)
                .RequiresHitProxies(false)
                .CreatePhysicsScene(true)
                .CreateNavigation(false)
                .CreateAISystem(false)
                .ShouldSimulatePhysics(false)
                .SetTransactional(false),
            false);
        return World;
    }

    bool BuildSocketOnlyPlan(uint64 DensityRevision, FShoringLayoutPlan& OutPlan, FString& OutError)
    {
        OutPlan = FShoringLayoutPlan{};
        OutPlan.PlanId = FGuid::NewGuid();
        OutPlan.SourceDensityRevision = DensityRevision;

        FShoringPieceSpec First;
        First.PieceId = FGuid::NewGuid();
        First.PlanId = OutPlan.PlanId;
        First.Kind = EShoringPieceKind::Upright;
        First.Transform = FTransform(FRotator::ZeroRotator, FVector(200.0f, 0.0f, 0.0f));
        First.HalfExtentsCm = FVector(10.0f);
        First.SupportAnchor.AnchorId = FGuid::NewGuid();
        First.SupportAnchor.Kind = EShoringAnchorKind::PieceSocket;
        First.SupportAnchor.PositionCm = First.Transform.GetLocation();
        First.SupportAnchor.Normal = FVector::UpVector;
        First.SupportAnchor.SourceDensityRevision = DensityRevision;

        FShoringPieceSpec Second;
        Second.PieceId = FGuid::NewGuid();
        Second.PlanId = OutPlan.PlanId;
        Second.Kind = EShoringPieceKind::Upright;
        Second.Transform = FTransform(FRotator::ZeroRotator, FVector(400.0f, 0.0f, 0.0f));
        Second.HalfExtentsCm = FVector(10.0f);
        Second.SupportAnchor.AnchorId = FGuid::NewGuid();
        Second.SupportAnchor.Kind = EShoringAnchorKind::PieceSocket;
        Second.SupportAnchor.PositionCm = Second.Transform.GetLocation();
        Second.SupportAnchor.Normal = FVector::UpVector;
        Second.SupportAnchor.SourceDensityRevision = DensityRevision;

        First.SupportAnchor.ConnectedPieceId = Second.PieceId;
        First.ConnectedPieceIds.Add(Second.PieceId);
        Second.SupportAnchor.ConnectedPieceId = First.PieceId;
        Second.ConnectedPieceIds.Add(First.PieceId);

        OutPlan.Pieces.Add(First);
        OutPlan.Pieces.Add(Second);
        OutPlan.bValid = true;
        return OutPlan.Validate(OutError);
    }

    int32 CountKind(const FShoringLayoutPlan& Plan, EShoringPieceKind Kind)
    {
        int32 Count = 0;
        for (const FShoringPieceSpec& Piece : Plan.Pieces)
        {
            Count += Piece.Kind == Kind ? 1 : 0;
        }
        return Count;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShoringPlannerBuildsBoundedLayoutTest,
    "MyProject2.World.Shoring.PlannerBuildsBoundedReciprocalLayout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShoringPlannerBuildsBoundedLayoutTest::RunTest(const FString& Parameters)
{
    FShoringLayoutRequest Request;
    Request.TrenchStartCm = FVector(0.0f, 0.0f, 200.0f);
    Request.TrenchEndCm = FVector(300.0f, 0.0f, 200.0f);
    Request.TrenchWidthCm = 200.0f;
    Request.TrenchDepthCm = 200.0f;
    Request.StationSpacingCm = 150.0f;
    Request.SourceDensityRevision = 42;
    Request.bIncludePlatform = true;
    Request.bIncludeLadder = true;

    FShoringLayoutPlan Plan;
    FString Error;
    TestTrue(TEXT("Planner should build a bounded shoring layout"),
        FShoringLayoutPlanner::BuildPlan(Request, Plan, Error));
    if (!Error.IsEmpty())
    {
        AddError(Error);
        return false;
    }

    TestTrue(TEXT("Plan should pass the full plan contract"), Plan.Validate(Error));
    TestEqual(TEXT("The plan keeps its source density revision"), static_cast<int64>(Plan.SourceDensityRevision), static_cast<int64>(42));
    TestEqual(TEXT("Three stations have six uprights"), CountKind(Plan, EShoringPieceKind::Upright), 6);
    TestEqual(TEXT("Two sides, two intervals and four courses create sixteen lagging boards"),
        CountKind(Plan, EShoringPieceKind::Lagging), 16);
    TestEqual(TEXT("One strut is generated at each station"), CountKind(Plan, EShoringPieceKind::Strut), 3);
    TestEqual(TEXT("The optional platform is generated"), CountKind(Plan, EShoringPieceKind::Platform), 1);
    TestEqual(TEXT("The optional ladder is generated"), CountKind(Plan, EShoringPieceKind::Ladder), 1);
    TestEqual(TEXT("Piece count remains within the configured budget"), Plan.Pieces.Num(), 27);

    TSet<FGuid> UniquePieceIds;
    int32 SurfaceAnchorCount = 0;
    for (const FShoringPieceSpec& Piece : Plan.Pieces)
    {
        TestTrue(TEXT("Every generated piece has a unique GUID"), !UniquePieceIds.Contains(Piece.PieceId));
        UniquePieceIds.Add(Piece.PieceId);
        TestTrue(TEXT("Every piece has finite geometry and an anchor"), Piece.Validate(Error));
        SurfaceAnchorCount += Piece.SupportAnchor.Kind == EShoringAnchorKind::VoxelSurface ? 1 : 0;
        for (const FGuid& ConnectedId : Piece.ConnectedPieceIds)
        {
            const FShoringPieceSpec* ConnectedPiece = Plan.Pieces.FindByPredicate(
                [&ConnectedId](const FShoringPieceSpec& Candidate) { return Candidate.PieceId == ConnectedId; });
            TestNotNull(TEXT("Connection target must exist in the plan"), ConnectedPiece);
            if (ConnectedPiece)
            {
                TestTrue(TEXT("Connections must be reciprocal"),
                    ConnectedPiece->ConnectedPieceIds.Contains(Piece.PieceId));
            }
        }
    }
    TestEqual(TEXT("Each station has one voxel wall anchor per side"), SurfaceAnchorCount, 6);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShoringPlannerRejectsInvalidBudgetTest,
    "MyProject2.World.Shoring.InvalidDimensionsAndBudgetFailClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShoringPlannerRejectsInvalidBudgetTest::RunTest(const FString& Parameters)
{
    FShoringLayoutRequest Request;
    Request.TrenchWidthCm = 0.0f;
    FShoringLayoutPlan Plan;
    FString Error;
    TestFalse(TEXT("Zero-width trench must be rejected"), FShoringLayoutPlanner::BuildPlan(Request, Plan, Error));
    TestTrue(TEXT("Rejected dimensions should explain the failure"), !Error.IsEmpty());

    Request = FShoringLayoutRequest{};
    Request.TrenchStartCm = FVector::ZeroVector;
    Request.TrenchEndCm = FVector(10000.0f, 0.0f, 0.0f);
    Request.StationSpacingCm = 50.0f;
    Request.MaxStations = 10;
    Error.Reset();
    TestFalse(TEXT("Excessive station count must be rejected before allocation"),
        FShoringLayoutPlanner::BuildPlan(Request, Plan, Error));
    TestTrue(TEXT("Rejected station budget should explain the failure"), !Error.IsEmpty());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShoringRejectsStaleDensityCommitTest,
    "MyProject2.World.Shoring.CommitRejectsStaleDensityRevision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShoringRejectsStaleDensityCommitTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateShoringTestWorld(TEXT("ShoringStaleRevisionTestWorld"));
    if (!TestNotNull(TEXT("Test world should be created"), World))
    {
        return false;
    }

    UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>();
    UExcavationShoringSubsystem* Shoring = World->GetSubsystem<UExcavationShoringSubsystem>();
    if (!TestNotNull(TEXT("Density owner should be created"), Volume)
        || !TestNotNull(TEXT("Shoring subsystem should be created"), Shoring))
    {
        World->DestroyWorld(false);
        return false;
    }

    FShoringLayoutRequest Request;
    Request.TrenchStartCm = FVector(0.0f, 0.0f, 200.0f);
    Request.TrenchEndCm = FVector(300.0f, 0.0f, 200.0f);
    Request.SourceDensityRevision = Volume->GetDensityRevision() + 1;

    FShoringLayoutPlan Plan;
    FString Error;
    TestTrue(TEXT("A geometry preview can be assembled before its revision gate"), FShoringLayoutPlanner::BuildPlan(Request, Plan, Error));

    TArray<APlaceableShoringPiece*> Pieces;
    TestFalse(TEXT("Commit must reject a plan created against a stale voxel revision"),
        Shoring->CommitPlan(Plan, Pieces, Error));
    TestEqual(TEXT("Stale commit returns no actors"), Pieces.Num(), 0);

    int32 SpawnedCount = 0;
    for (TActorIterator<APlaceableShoringPiece> It(World); It; ++It)
    {
        ++SpawnedCount;
    }
    TestEqual(TEXT("Stale commit must not leave partial shoring actors"), SpawnedCount, 0);
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShoringPreviewMoveRevalidateRemoveTest,
    "MyProject2.World.Shoring.PreviewMoveRevalidateAndRemovePiece",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShoringPreviewMoveRevalidateRemoveTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateShoringTestWorld(TEXT("ShoringPieceLifecycleTestWorld"));
    if (!TestNotNull(TEXT("Test world should be created"), World))
    {
        return false;
    }

    UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>();
    UExcavationShoringSubsystem* Shoring = World->GetSubsystem<UExcavationShoringSubsystem>();
    if (!TestNotNull(TEXT("Density owner should be created"), Volume)
        || !TestNotNull(TEXT("Shoring subsystem should be created"), Shoring))
    {
        World->DestroyWorld(false);
        return false;
    }

    FShoringLayoutPlan Plan;
    FString Error;
    TestTrue(TEXT("Socket-only fixture plan should be valid"), BuildSocketOnlyPlan(Volume->GetDensityRevision(), Plan, Error));
    if (!Error.IsEmpty())
    {
        AddError(Error);
        World->DestroyWorld(false);
        return false;
    }

    TArray<APlaceableShoringPiece*> PreviewPieces;
    TestTrue(TEXT("Plan preview should spawn non-colliding preview actors"), Shoring->PreviewPlan(Plan, PreviewPieces, Error));
    TestEqual(TEXT("Both preview members are returned"), PreviewPieces.Num(), 2);
    if (!PreviewPieces.IsEmpty())
    {
        TestEqual(TEXT("Preview actors remain in Planned state"),
            static_cast<uint8>(PreviewPieces[0]->GetPieceState()),
            static_cast<uint8>(EShoringPieceState::Planned));
    }

    TArray<APlaceableShoringPiece*> CommittedPieces;
    TestTrue(TEXT("The plan can commit atomically after validation"),
        Shoring->CommitPlan(Plan, CommittedPieces, Error));
    TestEqual(TEXT("Both members are committed"), CommittedPieces.Num(), 2);
    if (CommittedPieces.Num() != 2)
    {
        World->DestroyWorld(false);
        return false;
    }

    const FGuid FirstId = CommittedPieces[0]->GetPieceId();
    const FGuid SecondId = CommittedPieces[1]->GetPieceId();
    FTransform MovedTransform = CommittedPieces[0]->GetActorTransform();
    MovedTransform.SetLocation(MovedTransform.GetLocation() + FVector(500.0f, 0.0f, 0.0f));

    FShoringSupportAnchor MovedAnchor = CommittedPieces[0]->GetSupportAnchor();
    MovedAnchor.AnchorId = FGuid::NewGuid();
    MovedAnchor.PositionCm = MovedTransform.GetLocation();
    MovedAnchor.SourceDensityRevision = Volume->GetDensityRevision();
    MovedAnchor.ConnectedPieceId = SecondId;
    TestTrue(TEXT("A socket-connected piece can be repositioned"),
        Shoring->MovePiece(FirstId, MovedTransform, MovedAnchor, Error));
    TestEqual(TEXT("Moving one member invalidates its structure plan"),
        static_cast<uint8>(Shoring->FindPiece(SecondId)->GetPieceState()),
        static_cast<uint8>(EShoringPieceState::NeedsRevalidation));

    TestTrue(TEXT("An intact socket graph can be revalidated"), Shoring->RevalidatePlan(Plan.PlanId, Error));
    TestEqual(TEXT("Successful revalidation restores active state"),
        static_cast<uint8>(Shoring->FindPiece(FirstId)->GetPieceState()),
        static_cast<uint8>(EShoringPieceState::Active));

    TestTrue(TEXT("A member can be removed by GUID"), Shoring->RemovePiece(FirstId, Error));
    TestNull(TEXT("Removed member is no longer registered"), Shoring->FindPiece(FirstId));
    TestFalse(TEXT("A remaining member with a missing primary socket cannot be silently revalidated"),
        Shoring->RevalidatePlan(Plan.PlanId, Error));
    TestTrue(TEXT("The remaining committed plan can be removed"), Shoring->RemovePlan(Plan.PlanId, Error));
    TestNull(TEXT("The second piece is also removed"), Shoring->FindPiece(SecondId));

    World->DestroyWorld(false);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS