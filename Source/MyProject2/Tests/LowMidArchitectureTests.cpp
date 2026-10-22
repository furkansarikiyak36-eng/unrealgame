#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Shoring/ShoringLayoutPlannerSubsystem.h"
#include "Shoring/PlaceableShoringPiece.h"
#include "WorldCore/VoxelRuntime/WorldRuntimeProfile.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShoringLayoutPlannerBuildsExpectedPieceFamilies,
    "MyProject2.Voxel.Shoring.LayoutPlanner.BuildsPieceFamilies",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShoringLayoutPlannerBuildsExpectedPieceFamilies::RunTest(const FString& Parameters)
{
    UShoringLayoutPlannerSubsystem* Planner = NewObject<UShoringLayoutPlannerSubsystem>();
    TestNotNull(TEXT("Planner object created"), Planner);
    if (!Planner) return false;

    TArray<FShoringPlacementCandidate> Candidates;
    FString Failure;
    const bool bBuilt = Planner->BuildTrenchPlan(
        FVector(0.0f, 0.0f, 400.0f),
        FVector(1200.0f, 0.0f, 400.0f),
        200.0f, 400.0f, 150.0f,
        Candidates, Failure);

    TestTrue(FString::Printf(TEXT("Valid plan built (%s)"), *Failure), bBuilt);
    TestTrue(TEXT("Plan is not empty"), Candidates.Num() > 0);

    int32 Uprights = 0;
    int32 Boards = 0;
    int32 Struts = 0;
    for (const FShoringPlacementCandidate& Candidate : Candidates)
    {
        switch (Candidate.PieceType)
        {
        case EShoringPieceType::Upright: ++Uprights; break;
        case EShoringPieceType::WallBoard: ++Boards; break;
        case EShoringPieceType::Strut: ++Struts; break;
        default: break;
        }
        TestTrue(TEXT("Candidate has positive dimensions"),
            Candidate.DimensionsCm.X > 0.0f && Candidate.DimensionsCm.Y > 0.0f && Candidate.DimensionsCm.Z > 0.0f);
        TestTrue(TEXT("Candidate has an anchor ID"), !Candidate.AnchorId.IsNone());
    }
    TestTrue(TEXT("Plan includes upright posts"), Uprights >= 2);
    TestTrue(TEXT("Plan includes wall boards"), Boards > 0);
    TestTrue(TEXT("Plan includes cross struts"), Struts > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FShoringLayoutPlannerRejectsInvalidDimensions,
    "MyProject2.Voxel.Shoring.LayoutPlanner.RejectsInvalidDimensions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShoringLayoutPlannerRejectsInvalidDimensions::RunTest(const FString& Parameters)
{
    UShoringLayoutPlannerSubsystem* Planner = NewObject<UShoringLayoutPlannerSubsystem>();
    if (!Planner) return false;

    TArray<FShoringPlacementCandidate> Candidates;
    FString Failure;
    const bool bBuilt = Planner->BuildTrenchPlan(
        FVector::ZeroVector, FVector(1000.0f, 0.0f, 0.0f),
        0.0f, 400.0f, 150.0f, Candidates, Failure);
    TestFalse(TEXT("Zero width is rejected"), bBuilt);
    TestTrue(TEXT("Failure is explained"), !Failure.IsEmpty());
    TestTrue(TEXT("Invalid plan returns no candidates"), Candidates.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldRuntimeProfileDefaultsAreConsistent,
    "MyProject2.Voxel.RuntimeProfile.DefaultsAreConsistent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldRuntimeProfileDefaultsAreConsistent::RunTest(const FString& Parameters)
{
    const UWorldRuntimeProfileSettings& Settings = UWorldRuntimeProfileSettings::Get();
    TestTrue(TEXT("Cell spacing is finite and positive"),
        FMath::IsFinite(Settings.GetCellSizeCm()) && Settings.GetCellSizeCm() > 0.0f);
    TestTrue(TEXT("Chunk cell count is valid"), Settings.GetCellsPerChunkAxis() >= 4);
    TestTrue(TEXT("Chunk world size is finite and positive"),
        FMath::IsFinite(Settings.GetChunkSizeCm()) && Settings.GetChunkSizeCm() > 0.0f);
    TestTrue(TEXT("Sampling budget is positive"), Settings.GetMaxDensitySamplesPerTick() > 0);
    TestTrue(TEXT("Mesh job budget is positive"), Settings.GetMaxMeshJobsInFlight() > 0);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
