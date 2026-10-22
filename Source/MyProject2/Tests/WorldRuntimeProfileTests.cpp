// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "WorldCore/WorldRuntimeProfile.h"
#include "WorldCore/WorldRuntimeProfileSubsystem.h"
#include "WorldCore/WorldSystemContracts.h"
#include "Misc/AutomationTest.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldRuntimeProfileDefaultsTest,
    "MyProject2.World.RuntimeProfile.DefaultProfilesValidate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldRuntimeProfileDefaultsTest::RunTest(const FString& Parameters)
{
    UWorldRuntimeProfileDataAsset* Defaults = NewObject<UWorldRuntimeProfileDataAsset>(GetTransientPackage());
    if (!TestNotNull(TEXT("Default profile asset should be created"), Defaults))
    {
        return false;
    }

    FString Error = TEXT("stale error should be cleared");
    TestTrue(TEXT("LOW_MID defaults should validate"), Defaults->LowMid.Validate(&Error));
    TestTrue(TEXT("Successful profile validation should clear the previous error"), Error.IsEmpty());
    TestTrue(TEXT("LOW_MID grid spacing should match chunk size"), Defaults->LowMid.HasConsistentChunkGrid());
    TestTrue(TEXT("HIGH_END defaults should validate"), Defaults->HighEnd.Validate(&Error));
    TestTrue(TEXT("HIGH_END grid spacing should match chunk size"), Defaults->HighEnd.HasConsistentChunkGrid());

    TestTrue(
        TEXT("HIGH_END should use finer near-field spacing"),
        Defaults->HighEnd.VoxelSampleSpacingCm < Defaults->LowMid.VoxelSampleSpacingCm);

    TestTrue(
        TEXT("HIGH_END should budget at least as many active editable chunks"),
        Defaults->HighEnd.MaxActiveEditableChunks >= Defaults->LowMid.MaxActiveEditableChunks);

    TestTrue(
        TEXT("HIGH_END should allow more mesh work than LOW_MID"),
        Defaults->HighEnd.MaxMeshJobsInFlight > Defaults->LowMid.MaxMeshJobsInFlight);

    FWorldRuntimeProfileValues InvalidSettings = Defaults->LowMid;
    InvalidSettings.CellsPerChunkAxis = 16;
    TestFalse(
        TEXT("Inconsistent spacing/chunk grid should fail validation"),
        InvalidSettings.Validate(&Error));
    TestFalse(TEXT("Invalid settings should provide a reason"), Error.IsEmpty());

    InvalidSettings = Defaults->LowMid;
    InvalidSettings.MaxQueuedMeshJobs = 0;
    TestFalse(
        TEXT("A zero mesh queue capacity should fail validation"),
        InvalidSettings.Validate(&Error));
    TestFalse(TEXT("Queue validation should provide a reason"), Error.IsEmpty());

    Defaults->ActiveProfile = static_cast<EWorldRuntimeProfile>(255);
    TestFalse(
        TEXT("An unsupported profile enum value should be rejected"),
        Defaults->ValidateActiveSettings(Error));
    TestFalse(TEXT("Invalid profile enum should provide a reason"), Error.IsEmpty());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldRuntimeProfileSubsystemLockTest,
    "MyProject2.World.RuntimeProfile.ConfigureAndLock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldRuntimeProfileSubsystemLockTest::RunTest(const FString& Parameters)
{
    UWorldRuntimeProfileDataAsset* Asset = NewObject<UWorldRuntimeProfileDataAsset>(GetTransientPackage());
    UWorldRuntimeProfileSubsystem* Runtime = NewObject<UWorldRuntimeProfileSubsystem>(GetTransientPackage());

    if (!TestNotNull(TEXT("Runtime profile asset should be created"), Asset)
        || !TestNotNull(TEXT("Runtime profile subsystem object should be created"), Runtime))
    {
        return false;
    }

    FString Error;
    TestFalse(TEXT("Profile should begin unlocked"), Runtime->IsProfileLocked());
    TestTrue(
        TEXT("A valid LOW_MID profile should configure before the lock"),
        Runtime->ConfigureProfile(Asset, Error));
    TestEqual(
        TEXT("The selected profile should be LOW_MID"),
        static_cast<uint8>(Runtime->GetActiveProfile()),
        static_cast<uint8>(EWorldRuntimeProfile::LowMid));
    TestTrue(TEXT("Successful configuration should clear the error"), Error.IsEmpty());

    Asset->ActiveProfile = EWorldRuntimeProfile::HighEnd;
    TestTrue(
        TEXT("Changing to HIGH_END should be allowed before locking"),
        Runtime->ConfigureProfile(Asset, Error));
    TestEqual(
        TEXT("The selected profile should now be HIGH_END"),
        static_cast<uint8>(Runtime->GetActiveProfile()),
        static_cast<uint8>(EWorldRuntimeProfile::HighEnd));

    Runtime->LockProfileForSession();
    TestTrue(TEXT("LockProfileForSession should lock the profile"), Runtime->IsProfileLocked());

    Asset->ActiveProfile = EWorldRuntimeProfile::LowMid;
    TestFalse(
        TEXT("Profile reconfiguration should fail after the session lock"),
        Runtime->ConfigureProfile(Asset, Error));
    TestFalse(TEXT("A rejected post-lock configuration should explain why"), Error.IsEmpty());
    TestEqual(
        TEXT("Rejected reconfiguration must not change the active profile"),
        static_cast<uint8>(Runtime->GetActiveProfile()),
        static_cast<uint8>(EWorldRuntimeProfile::HighEnd));

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldSystemContractsValidationTest,
    "MyProject2.World.Contracts.ValidateRequestsAndBuffers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldSystemContractsValidationTest::RunTest(const FString& Parameters)
{
    FString Error;

    FWorldVoxelEditRequest Edit;
    TestTrue(TEXT("Default sphere edit request should be valid"), Edit.Validate(Error));
    Edit.RadiusCm = 0.0f;
    TestFalse(TEXT("Sphere edit with zero radius should be rejected"), Edit.Validate(Error));
    TestFalse(TEXT("Invalid sphere edit should provide a reason"), Error.IsEmpty());

    Edit = FWorldVoxelEditRequest();
    Edit.Shape = EWorldVoxelEditShape::Box;
    Edit.HalfExtentCm = FVector(50.0f, -1.0f, 50.0f);
    TestFalse(TEXT("Box edit with a negative half-extent should be rejected"), Edit.Validate(Error));

    FWorldSurfaceSampleRequest Surface;
    TestTrue(TEXT("Default surface sample request should be valid"), Surface.Validate(Error));
    Surface.SamplesX = 6;
    Surface.SamplesY = 6;
    Surface.MaxSamples = 25;
    TestFalse(TEXT("Surface sample grid larger than MaxSamples should be rejected"), Surface.Validate(Error));

    FWorldShapeTraceRequest ShapeTrace;
    TestTrue(TEXT("Default sphere trace request should be valid"), ShapeTrace.Validate(Error));
    ShapeTrace.Shape = EWorldShapeTraceKind::Capsule;
    ShapeTrace.RadiusCm = 30.0f;
    ShapeTrace.HalfHeightCm = 20.0f;
    TestFalse(TEXT("Capsule half-height smaller than radius should be rejected"), ShapeTrace.Validate(Error));

    FWorldDensityChunkSnapshot Snapshot;
    Snapshot.CellsPerChunkAxis = 2;
    Snapshot.BorderCells = 1;
    Snapshot.SampleSpacingCm = 25.0f;
    TestEqual(
        TEXT("Density snapshot count includes boundary planes and border samples"),
        Snapshot.GetExpectedDensitySampleCount(),
        static_cast<int64>(125));
    Snapshot.DensitySamples.SetNum(125);
    TestTrue(TEXT("Density snapshot with exact sample storage should validate"), Snapshot.HasValidSampleStorage());
    Snapshot.DensitySamples.SetNum(124);
    TestFalse(TEXT("Density snapshot with truncated sample storage should be rejected"), Snapshot.HasValidSampleStorage());

    FWorldVoxelMeshData Mesh;
    Mesh.Vertices = { FVector(0, 0, 0), FVector(1, 0, 0), FVector(0, 1, 0) };
    Mesh.Normals = { FVector::UpVector, FVector::UpVector, FVector::UpVector };
    Mesh.UV0 = { FVector2D(0, 0), FVector2D(1, 0), FVector2D(0, 1) };
    Mesh.TriangleIndices = { 0, 1, 2 };
    Mesh.TriangleMaterialIds = { 0 };
    TestTrue(TEXT("Well-formed triangle mesh data should validate"), Mesh.IsTopologyValid());

    Mesh.TriangleIndices[2] = 3;
    TestFalse(TEXT("Mesh indices beyond the vertex array should be rejected"), Mesh.IsTopologyValid());
    Mesh.TriangleIndices[2] = 2;
    Mesh.TriangleMaterialIds.Reset();
    Mesh.TriangleMaterialIds.Add(0);
    Mesh.TriangleMaterialIds.Add(1);
    TestFalse(TEXT("Material IDs must be empty or supplied once per triangle"), Mesh.IsTopologyValid());

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
