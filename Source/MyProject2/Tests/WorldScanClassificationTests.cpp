// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "WorldCore/WorldClassificationSubsystem.h"
#include "WorldCore/WorldScanSubsystem.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

namespace
{
    UWorld* CreateScanTestWorld(const TCHAR* Name)
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldClassificationTagPolicyTest,
    "MyProject2.World.Classification.RegionTagPolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldClassificationTagPolicyTest::RunTest(const FString& Parameters)
{
    EWorldRegionKind Kind = EWorldRegionKind::Unknown;
    EWorldClassificationSource Source = EWorldClassificationSource::DefaultUnknown;

    TestTrue(
        TEXT("EditableVoxel region marker should parse"),
        UWorldClassificationSubsystem::TryClassifyRegionTag(
            FName(TEXT("World.Region.EditableVoxel")), Kind, Source));
    TestEqual(TEXT("EditableVoxel marker kind"), static_cast<uint8>(Kind),
        static_cast<uint8>(EWorldRegionKind::EditableVoxel));
    TestEqual(TEXT("Actor region marker source"), static_cast<uint8>(Source),
        static_cast<uint8>(EWorldClassificationSource::ActorTag));

    TestTrue(
        TEXT("Landscape layer marker should parse"),
        UWorldClassificationSubsystem::TryClassifyRegionTag(
            FName(TEXT("World.LandscapeLayer.StaticGeometry")), Kind, Source));
    TestEqual(TEXT("Landscape layer marker kind"), static_cast<uint8>(Kind),
        static_cast<uint8>(EWorldRegionKind::StaticGeometry));
    TestEqual(TEXT("Landscape layer marker source"), static_cast<uint8>(Source),
        static_cast<uint8>(EWorldClassificationSource::LandscapeLayer));

    TestFalse(
        TEXT("Object classification tags must not be mistaken for terrain region tags"),
        UWorldClassificationSubsystem::TryClassifyRegionTag(
            FName(TEXT("World.Object.Rock")), Kind, Source));
    TestEqual(TEXT("Unknown tags should reset kind"), static_cast<uint8>(Kind),
        static_cast<uint8>(EWorldRegionKind::Unknown));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldClassificationManualOverrideGridTest,
    "MyProject2.World.Classification.ManualOverrideGrid",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldClassificationManualOverrideGridTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateScanTestWorld(TEXT("ClassificationManualOverrideTestWorld"));
    if (!TestNotNull(TEXT("Classification test world should be created"), World))
    {
        return false;
    }

    UWorldClassificationSubsystem* Classifier = World->GetSubsystem<UWorldClassificationSubsystem>();
    if (!TestNotNull(TEXT("World classification subsystem should be created"), Classifier))
    {
        World->DestroyWorld(false);
        return false;
    }

    TestTrue(
        TEXT("A full-cell EditableVoxel override should be accepted"),
        Classifier->SetManualOverride(
            FName(TEXT("EditFull")),
            FBox(FVector(0, 0, 0), FVector(800, 800, 800)),
            EWorldRegionKind::EditableVoxel));
    TestTrue(
        TEXT("A partial-cell EditableVoxel override should be accepted"),
        Classifier->SetManualOverride(
            FName(TEXT("EditPartial")),
            FBox(FVector(800, 0, 0), FVector(1200, 800, 800)),
            EWorldRegionKind::EditableVoxel));
    TestTrue(
        TEXT("Conflicting full-cell EditableVoxel override should be accepted for conflict testing"),
        Classifier->SetManualOverride(
            FName(TEXT("EditConflict")),
            FBox(FVector(1600, 0, 0), FVector(2400, 800, 800)),
            EWorldRegionKind::EditableVoxel));
    TestTrue(
        TEXT("Conflicting full-cell StaticGeometry override should be accepted for conflict testing"),
        Classifier->SetManualOverride(
            FName(TEXT("StaticConflict")),
            FBox(FVector(1600, 0, 0), FVector(2400, 800, 800)),
            EWorldRegionKind::StaticGeometry));
    TestTrue(
        TEXT("Final full-cell StaticGeometry override should be accepted"),
        Classifier->SetManualOverride(
            FName(TEXT("StaticFull")),
            FBox(FVector(2400, 0, 0), FVector(3200, 800, 800)),
            EWorldRegionKind::StaticGeometry));

    FWorldClassificationBuildRequest Request;
    Request.Bounds = FBox(FVector(0, 0, 0), FVector(3200, 800, 800));
    Request.RegionSizeCm = FVector(800, 800, 800);
    Request.MaxRegions = 4;
    Request.MaxGeometrySamples = 0;
    Request.bIncludeGeometrySampling = false;

    FWorldClassificationBuildResult Result;
    TestTrue(
        TEXT("Valid classification request should build the region map"),
        Classifier->BuildClassification(Request, Result));
    TestTrue(TEXT("Build result should be marked successful"), Result.bSucceeded);
    TestEqual(TEXT("Four 8 m cells should be built"), Result.Regions.Num(), 4);
    TestEqual(TEXT("No region should remain unknown when all cells are overridden"),
        Result.UnknownRegionCount, 0);

    FWorldRegionClassification Region;
    TestTrue(TEXT("First cell lookup should succeed"),
        Classifier->QueryClassification(FVector(400, 400, 400), Region));
    TestEqual(TEXT("The first cell should be editable"),
        static_cast<uint8>(Region.Kind), static_cast<uint8>(EWorldRegionKind::EditableVoxel));
    TestEqual(TEXT("Manual override should be the source"),
        static_cast<uint8>(Region.Source), static_cast<uint8>(EWorldClassificationSource::ManualOverride));

    TestTrue(TEXT("Second cell lookup should succeed"),
        Classifier->QueryClassification(FVector(1200, 400, 400), Region));
    TestEqual(TEXT("An override covering only part of a coarse cell must become Transition"),
        static_cast<uint8>(Region.Kind), static_cast<uint8>(EWorldRegionKind::Transition));

    TestTrue(TEXT("Third cell lookup should succeed"),
        Classifier->QueryClassification(FVector(2000, 400, 400), Region));
    TestEqual(TEXT("Conflicting same-priority markers must fail closed as Transition"),
        static_cast<uint8>(Region.Kind), static_cast<uint8>(EWorldRegionKind::Transition));
    TestTrue(TEXT("Conflict reason should be explicit"), Region.Reason.Contains(TEXT("Conflicting equal-priority")));

    TestTrue(TEXT("Fourth cell lookup should succeed"),
        Classifier->QueryClassification(FVector(2800, 400, 400), Region));
    TestEqual(TEXT("The fourth cell should remain static geometry"),
        static_cast<uint8>(Region.Kind), static_cast<uint8>(EWorldRegionKind::StaticGeometry));

    TestTrue(TEXT("Manual override can be removed"), Classifier->RemoveManualOverride(FName(TEXT("EditFull"))));
    TestFalse(TEXT("Mutating overrides should invalidate the built map"),
        Classifier->QueryClassification(FVector(400, 400, 400), Region));

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldScanDemandDrivenQueriesTest,
    "MyProject2.World.Scan.DemandDrivenQueries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldScanDemandDrivenQueriesTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateScanTestWorld(TEXT("WorldScanDemandDrivenTestWorld"));
    if (!TestNotNull(TEXT("Scan test world should be created"), World))
    {
        return false;
    }

    UWorldScanSubsystem* Scanner = World->GetSubsystem<UWorldScanSubsystem>();
    if (!TestNotNull(TEXT("World scan subsystem should be created"), Scanner))
    {
        World->DestroyWorld(false);
        return false;
    }

    FWorldScanQuery Ray;
    Ray.World = World;
    Ray.StartCm = FVector(0, 0, 500);
    Ray.EndCm = FVector(0, 0, -500);
    FWorldScanHit Hit;
    TestFalse(TEXT("An empty test world should not produce a ray hit"), Scanner->Trace(Ray, Hit));
    TestFalse(TEXT("Missed ray should leave bHit false"), Hit.bHit);

    FWorldShapeTraceRequest Shape;
    Shape.RadiusCm = 0.0f;
    FWorldShapeTraceResult ShapeResult;
    TestFalse(TEXT("Invalid shape parameters should be rejected"), Scanner->ShapeTrace(
        World, Shape, nullptr, ShapeResult));
    TestFalse(TEXT("Rejected shape trace must not report success"), ShapeResult.bSucceeded);

    Shape.RadiusCm = 25.0f;
    TestTrue(TEXT("A valid shape query should execute even without a hit"),
        Scanner->ShapeTrace(World, Shape, nullptr, ShapeResult));
    TestTrue(TEXT("A valid no-hit shape query is still a completed query"), ShapeResult.bSucceeded);
    TestEqual(TEXT("Empty world should return no swept hits"), ShapeResult.Hits.Num(), 0);

    FWorldOverlapRequest Overlap;
    FWorldOverlapResult OverlapResult;
    TestTrue(TEXT("Valid sphere overlap should complete"), Scanner->QueryOverlaps(
        World, Overlap, nullptr, OverlapResult));
    TestTrue(TEXT("Completed overlap request should set bSucceeded"), OverlapResult.bSucceeded);

    FWorldSurfaceSampleRequest Surface;
    FWorldSurfaceSampleResult SurfaceResult;
    TestFalse(TEXT("Surface sample with no collidable surface should report no hits"),
        Scanner->SampleSurface(World, Surface, nullptr, SurfaceResult));
    TestFalse(TEXT("Empty surface sample should not claim success"), SurfaceResult.bSucceeded);

    World->DestroyWorld(false);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
