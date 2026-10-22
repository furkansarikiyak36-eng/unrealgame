// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "WorldCore/WorldClassificationSubsystem.h"
#include "WorldCore/WorldVoxelInteractionSubsystem.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

namespace
{
    UWorld* CreateVoxelFootprintTestWorld(const TCHAR* Name)
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

    bool BuildEditableAndStaticSplit(UWorld* World, FString& OutError)
    {
        UWorldClassificationSubsystem* Classification =
            World ? World->GetSubsystem<UWorldClassificationSubsystem>() : nullptr;
        if (!Classification)
        {
            OutError = TEXT("Classification subsystem is unavailable.");
            return false;
        }

        const FBox MapBounds(FVector(-800.0f), FVector(800.0f));
        if (!Classification->SetManualOverride(
                FName(TEXT("EditableLeftHalf")),
                FBox(FVector(-800.0f, -800.0f, -800.0f), FVector(0.0f, 800.0f, 800.0f)),
                EWorldRegionKind::EditableVoxel,
                TEXT("Editable half for footprint test"))
            || !Classification->SetManualOverride(
                FName(TEXT("StaticRightHalf")),
                FBox(FVector(0.0f, -800.0f, -800.0f), FVector(800.0f, 800.0f, 800.0f)),
                EWorldRegionKind::StaticGeometry,
                TEXT("Static half for footprint test")))
        {
            OutError = TEXT("Could not create the editable/static region overrides.");
            return false;
        }

        FWorldClassificationBuildRequest BuildRequest;
        BuildRequest.Bounds = MapBounds;
        BuildRequest.RegionSizeCm = FVector(800.0f);
        BuildRequest.MaxRegions = 64;
        BuildRequest.bIncludeGeometrySampling = false;

        FWorldClassificationBuildResult BuildResult;
        if (!Classification->BuildClassification(BuildRequest, BuildResult))
        {
            OutError = BuildResult.Error.IsEmpty()
                ? TEXT("Split classification map failed to build.")
                : BuildResult.Error;
            return false;
        }
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldVoxelEditFootprintRejectsStaticNeighborTest,
    "MyProject2.World.VoxelInteraction.EditFootprintCannotCrossStaticRegion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldVoxelEditFootprintRejectsStaticNeighborTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateVoxelFootprintTestWorld(TEXT("VoxelFootprintStaticNeighborWorld"));
    if (!TestNotNull(TEXT("Test world should be created"), World))
    {
        return false;
    }

    UWorldVoxelInteractionSubsystem* Interaction =
        World->GetSubsystem<UWorldVoxelInteractionSubsystem>();
    UExcavationVolumeSubsystem* Excavation =
        World->GetSubsystem<UExcavationVolumeSubsystem>();
    UWorldClassificationSubsystem* Classification =
        World->GetSubsystem<UWorldClassificationSubsystem>();
    if (!TestNotNull(TEXT("Voxel interaction subsystem should be created"), Interaction)
        || !TestNotNull(TEXT("Authoritative density subsystem should be created"), Excavation)
        || !TestNotNull(TEXT("Classification subsystem should be created"), Classification))
    {
        World->DestroyWorld(false);
        return false;
    }

    FString SetupError;
    TestTrue(TEXT("Build a region map with editable and static halves"),
        BuildEditableAndStaticSplit(World, SetupError));
    if (!SetupError.IsEmpty())
    {
        AddError(SetupError);
        World->DestroyWorld(false);
        return false;
    }

    FWorldRegionClassification CenterRegion;
    TestTrue(TEXT("Sphere center should be in the editable half"),
        Classification->QueryClassification(FVector(-10.0f, 100.0f, 100.0f), CenterRegion));
    TestEqual(TEXT("Center classification is EditableVoxel"),
        static_cast<uint8>(CenterRegion.Kind),
        static_cast<uint8>(EWorldRegionKind::EditableVoxel));

    const int32 OverridesBefore = Excavation->GetOverrideCellCount();
    FWorldVoxelEditRequest Request;
    Request.OperationId = FGuid::NewGuid();
    Request.Shape = EWorldVoxelEditShape::Sphere;
    Request.Operation = EWorldVoxelEditOperation::Carve;
    Request.CenterCm = FVector(-10.0f, 100.0f, 100.0f);
    Request.RadiusCm = 50.0f; // AABB crosses X=0 into StaticGeometry.
    Request.TargetDensity = 0.0f;
    Request.MaxSamples = 1000;

    FWorldVoxelEditResult Result;
    TestFalse(
        TEXT("Edit must be rejected when its footprint crosses into StaticGeometry"),
        Interaction->ApplyEdit(Request, Result));
    TestTrue(TEXT("Rejection should explain the footprint conflict"), !Result.Error.IsEmpty());
    TestEqual(
        TEXT("Rejected cross-region edit must not mutate any density"),
        Excavation->GetOverrideCellCount(),
        OverridesBefore);

    World->DestroyWorld(false);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
