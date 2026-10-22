// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "WorldCore/WorldClassificationSubsystem.h"
#include "WorldCore/WorldVoxelInteractionSubsystem.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

namespace
{
    UWorld* CreateVoxelInteractionTestWorld(const TCHAR* Name)
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

    bool BuildEditableTestRegion(UWorld* World, FString& OutError)
    {
        UWorldClassificationSubsystem* Classification =
            World ? World->GetSubsystem<UWorldClassificationSubsystem>() : nullptr;
        if (!Classification)
        {
            OutError = TEXT("World classification subsystem is unavailable.");
            return false;
        }

        const FBox Bounds(FVector(-800.0f), FVector(800.0f));
        if (!Classification->SetManualOverride(
            FName(TEXT("VoxelInteractionEditableTestRegion")),
            Bounds,
            EWorldRegionKind::EditableVoxel,
            TEXT("Automation test region")))
        {
            OutError = TEXT("Manual EditableVoxel region was rejected.");
            return false;
        }

        FWorldClassificationBuildRequest BuildRequest;
        BuildRequest.Bounds = Bounds;
        BuildRequest.RegionSizeCm = FVector(800.0f);
        BuildRequest.MaxRegions = 64;
        BuildRequest.bIncludeGeometrySampling = false;

        FWorldClassificationBuildResult BuildResult;
        if (!Classification->BuildClassification(BuildRequest, BuildResult))
        {
            OutError = BuildResult.Error.IsEmpty()
                ? TEXT("EditableVoxel classification build failed.")
                : BuildResult.Error;
            return false;
        }
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldVoxelInteractionEditRollbackTest,
    "MyProject2.World.VoxelInteraction.EditRollbackAndDirtyChunks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldVoxelInteractionEditRollbackTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateVoxelInteractionTestWorld(TEXT("VoxelInteractionEditRollbackWorld"));
    if (!TestNotNull(TEXT("Test world should be created"), World))
    {
        return false;
    }

    UWorldVoxelInteractionSubsystem* Interaction =
        World->GetSubsystem<UWorldVoxelInteractionSubsystem>();
    UExcavationVolumeSubsystem* Excavation =
        World->GetSubsystem<UExcavationVolumeSubsystem>();
    if (!TestNotNull(TEXT("Voxel interaction subsystem should be created"), Interaction)
        || !TestNotNull(TEXT("Authoritative density subsystem should be created"), Excavation))
    {
        World->DestroyWorld(false);
        return false;
    }

    FString Error;
    TestTrue(TEXT("Build an explicitly editable test patch"), BuildEditableTestRegion(World, Error));
    if (!Error.IsEmpty())
    {
        AddError(Error);
        World->DestroyWorld(false);
        return false;
    }

    const int32 OverridesBefore = Excavation->GetOverrideCellCount();
    const uint64 RevisionBefore = Excavation->GetDensityRevision();

    FWorldVoxelEditRequest Request;
    Request.OperationId = FGuid::NewGuid();
    Request.Shape = EWorldVoxelEditShape::Sphere;
    Request.Operation = EWorldVoxelEditOperation::Carve;
    Request.CenterCm = FVector(100.0f, 100.0f, -100.0f);
    Request.RadiusCm = 50.0f;
    Request.TargetDensity = 0.0f;
    Request.MaxSamples = 1000;

    FWorldVoxelEditResult Result;
    TestTrue(TEXT("Authorized sphere carve should commit"), Interaction->ApplyEdit(Request, Result));
    TestTrue(TEXT("Successful edit returns a successful result"), Result.bSucceeded);
    TestTrue(TEXT("Result preserves the request transaction ID"), Result.OperationId == Request.OperationId);
    TestTrue(TEXT("Changed cells should be reported"), Result.ModifiedSampleCount > 0);
    TestTrue(TEXT("Changed edit should advance the density revision"), Excavation->GetDensityRevision() > RevisionBefore);
    TestTrue(TEXT("Dirty chunks should be returned"), Result.DirtyChunks.Chunks.Num() > 0);
    TestTrue(TEXT("Edit should create sparse density overrides"), Excavation->GetOverrideCellCount() > OverridesBefore);

    const int32 OverridesAfterEdit = Excavation->GetOverrideCellCount();
    FWorldVoxelEditResult DuplicateResult;
    TestFalse(TEXT("A duplicated operation ID must not apply a second edit"), Interaction->ApplyEdit(Request, DuplicateResult));
    TestTrue(TEXT("Duplicate rejection must not mutate density"), Excavation->GetOverrideCellCount() == OverridesAfterEdit);

    FWorldVoxelEditResult RollbackResult;
    TestTrue(TEXT("Latest unconflicted edit should roll back"), Interaction->RollbackEdit(Request.OperationId, RollbackResult));
    TestTrue(TEXT("Rollback returns a successful transaction result"), RollbackResult.bSucceeded);
    TestTrue(TEXT("Rollback reports the same request transaction ID"), RollbackResult.OperationId == Request.OperationId);
    TestTrue(TEXT("Rollback removes this edit's sparse overrides"), Excavation->GetOverrideCellCount() < OverridesAfterEdit);
    TestTrue(TEXT("Rollback also reports dirty chunks"), RollbackResult.DirtyChunks.Chunks.Num() > 0);

    FWorldVoxelEditResult ExpiredRollbackResult;
    TestFalse(TEXT("A second rollback of the same receipt must fail"), Interaction->RollbackEdit(Request.OperationId, ExpiredRollbackResult));

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldVoxelInteractionAuthorizationTest,
    "MyProject2.World.VoxelInteraction.UnknownAndInvalidEditsFailClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldVoxelInteractionAuthorizationTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateVoxelInteractionTestWorld(TEXT("VoxelInteractionAuthorizationWorld"));
    if (!TestNotNull(TEXT("Test world should be created"), World))
    {
        return false;
    }

    UWorldVoxelInteractionSubsystem* Interaction =
        World->GetSubsystem<UWorldVoxelInteractionSubsystem>();
    UExcavationVolumeSubsystem* Excavation =
        World->GetSubsystem<UExcavationVolumeSubsystem>();
    if (!TestNotNull(TEXT("Voxel interaction subsystem should be created"), Interaction)
        || !TestNotNull(TEXT("Authoritative density subsystem should be created"), Excavation))
    {
        World->DestroyWorld(false);
        return false;
    }

    const int32 Before = Excavation->GetOverrideCellCount();
    FWorldVoxelEditRequest UnknownRequest;
    UnknownRequest.OperationId = FGuid::NewGuid();
    UnknownRequest.Shape = EWorldVoxelEditShape::Sphere;
    UnknownRequest.Operation = EWorldVoxelEditOperation::Carve;
    UnknownRequest.CenterCm = FVector(5000.0f, 5000.0f, 5000.0f);
    UnknownRequest.RadiusCm = 50.0f;
    UnknownRequest.TargetDensity = 0.0f;

    FWorldVoxelEditResult UnknownResult;
    TestFalse(TEXT("Direct edits outside a known EditableVoxel region must be denied"),
        Interaction->ApplyEdit(UnknownRequest, UnknownResult));
    TestTrue(TEXT("Denied edit must provide a reason"), !UnknownResult.Error.IsEmpty());
    TestEqual(TEXT("Denied edit must not change sparse density"), Excavation->GetOverrideCellCount(), Before);

    FWorldVoxelEditRequest InvalidFill;
    InvalidFill.OperationId = FGuid::NewGuid();
    InvalidFill.Shape = EWorldVoxelEditShape::Sphere;
    InvalidFill.Operation = EWorldVoxelEditOperation::Fill;
    InvalidFill.CenterCm = FVector::ZeroVector;
    InvalidFill.RadiusCm = 50.0f;
    InvalidFill.TargetDensity = 0.0f;

    FWorldVoxelEditResult InvalidFillResult;
    TestFalse(TEXT("Fill must request solid target density 1.0"), Interaction->ApplyEdit(InvalidFill, InvalidFillResult));
    TestTrue(TEXT("Invalid fill density should be explained"), !InvalidFillResult.Error.IsEmpty());
    TestEqual(TEXT("Invalid fill must not mutate density"), Excavation->GetOverrideCellCount(), Before);

    FWorldPlacementQuery InvalidPlacement;
    InvalidPlacement.PieceHalfExtentCm = FVector(10.0f, 0.0f, 10.0f);
    FString ValidationError;
    TestFalse(TEXT("Placement contract rejects zero piece half-extent"), InvalidPlacement.Validate(ValidationError));
    TestTrue(TEXT("Invalid placement query returns a reason"), !ValidationError.IsEmpty());

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldVoxelInteractionPlacementRevisionTest,
    "MyProject2.World.VoxelInteraction.PlacementRejectsStaleRevision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldVoxelInteractionPlacementRevisionTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateVoxelInteractionTestWorld(TEXT("VoxelInteractionStalePlacementWorld"));
    if (!TestNotNull(TEXT("Test world should be created"), World))
    {
        return false;
    }

    UWorldVoxelInteractionSubsystem* Interaction =
        World->GetSubsystem<UWorldVoxelInteractionSubsystem>();
    UExcavationVolumeSubsystem* Excavation =
        World->GetSubsystem<UExcavationVolumeSubsystem>();
    if (!TestNotNull(TEXT("Voxel interaction subsystem should be created"), Interaction)
        || !TestNotNull(TEXT("Authoritative density subsystem should be created"), Excavation))
    {
        World->DestroyWorld(false);
        return false;
    }

    FWorldPlacementQuery Query;
    Query.HitPointCm = FVector(100.0f, 100.0f, 100.0f);
    Query.HitNormal = FVector::UpVector;
    Query.DesiredAxis = FVector::ForwardVector;
    Query.PieceHalfExtentCm = FVector(20.0f, 20.0f, 20.0f);
    Query.RequiredGroup = FName(TEXT("VoxelSurface"));
    Query.ExpectedWorldRevision = Excavation->GetDensityRevision() + 1;

    FWorldPlacementResult Result;
    TestFalse(TEXT("Placement must reject a preview captured against an old density revision"),
        Interaction->BuildPlacementPreview(Query, nullptr, Result));
    TestFalse(TEXT("Rejected stale placement must not report a valid preview"), Result.bValid);
    TestEqual(
        TEXT("Stale revision maps to the explicit stale-surface failure"),
        static_cast<uint8>(Result.Failure),
        static_cast<uint8>(EWorldPlacementFailure::StaleSurface));

    World->DestroyWorld(false);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
