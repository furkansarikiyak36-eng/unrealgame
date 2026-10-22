// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

namespace
{
    UWorld* CreateDensitySnapshotTestWorld(const TCHAR* Name)
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
                .CreatePhysicsScene(false)
                .CreateNavigation(false)
                .CreateAISystem(false)
                .ShouldSimulatePhysics(false)
                .SetTransactional(false),
            false);
        return World;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExcavationDensitySnapshotRevisionTest,
    "MyProject2.World.VoxelDensity.SnapshotRevisionAndGrid",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FExcavationDensitySnapshotRevisionTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateDensitySnapshotTestWorld(TEXT("ExcavationDensitySnapshotTestWorld"));
    if (!TestNotNull(TEXT("Test world should be created"), World))
    {
        return false;
    }

    UExcavationVolumeSubsystem* Excavation = World->GetSubsystem<UExcavationVolumeSubsystem>();
    if (!TestNotNull(TEXT("World should create the authoritative excavation subsystem"), Excavation))
    {
        World->DestroyWorld(false);
        return false;
    }

    FString Error;
    TestTrue(
        TEXT("LOW_MID grid should configure before the first edit"),
        Excavation->ConfigureGridForRuntimeProfile(25.0f, 32, Error));
    TestTrue(TEXT("Successful grid configuration should clear errors"), Error.IsEmpty());

    FWorldDensityChunkSnapshot Before;
    TestTrue(
        TEXT("A chunk snapshot should sample the configured base density"),
        Excavation->BuildChunkSnapshot(FIntVector(0, 0, -1), Before));
    TestTrue(TEXT("Snapshot storage should match its metadata"), Before.HasValidSampleStorage());
    TestEqual(TEXT("Snapshot includes one base material ID per density sample"),
        Before.MaterialIds.Num(), Before.DensitySamples.Num());
    TestEqual(TEXT("LOW_MID chunk uses 32 cells per axis"), Before.CellsPerChunkAxis, 32);
    TestEqual(TEXT("Snapshot contains one-cell mesh borders"), Before.BorderCells, 1);
    TestEqual(TEXT("32 cells plus boundary/border planes gives 35 samples per axis"),
        Before.DensitySamples.Num(), 35 * 35 * 35);

    const int32 CenterIndex = ((17 * 35) + 17) * 35 + 17;
    TestTrue(TEXT("Default scalar base should be fully solid before edits"),
        FMath::IsNearlyEqual(Before.DensitySamples[CenterIndex], 1.0f));

    const uint64 InitialRevision = Before.SourceRevision;
    FExcavationEditRecord EditRecord;
    const FVector CellCenter = Excavation->CellToWorldCenter(FIntVector(16, 16, -16));
    TestTrue(
        TEXT("Sphere carve should commit to authoritative sparse density"),
        Excavation->ApplySphereEdit(CellCenter, 40.0f, EExcavationEditType::Carve, EditRecord));
    TestTrue(TEXT("Carve should advance the authoritative density revision"),
        Excavation->GetDensityRevision() > InitialRevision);

    FWorldDensityChunkSnapshot After;
    TestTrue(TEXT("Post-edit chunk snapshot should rebuild"), Excavation->BuildChunkSnapshot(FIntVector(0, 0, -1), After));
    TestTrue(TEXT("Snapshot revision should advance after a committed edit"),
        After.SourceRevision > Before.SourceRevision);
    TestTrue(TEXT("Carved center sample should now be empty"),
        FMath::IsNearlyEqual(After.DensitySamples[CenterIndex], 0.0f));

    Error.Reset();
    TestFalse(
        TEXT("An existing edited grid must not be reinterpreted using HIGH_END coordinates"),
        Excavation->ConfigureGridForRuntimeProfile(12.5f, 64, Error));
    TestFalse(TEXT("Rejected grid reinterpretation should explain the mismatch"), Error.IsEmpty());
    TestTrue(TEXT("Rejected grid change must preserve the original cell spacing"),
        FMath::IsNearlyEqual(Excavation->CellSizeCm, 25.0f));
    TestEqual(TEXT("Rejected grid change must preserve the original chunk grid"),
        Excavation->CellsPerChunkAxis, 32);

    FExcavationVolumeSnapshot IncompatibleSave;
    TestTrue(TEXT("Current excavation state should be exportable for compatibility testing"),
        Excavation->ExportSnapshot(IncompatibleSave));
    IncompatibleSave.CellSizeCm = 12.5f;
    IncompatibleSave.CellsPerChunkAxis = 64;
    TestFalse(
        TEXT("Snapshot import must not override a runtime-locked voxel grid"),
        Excavation->ImportSnapshot(IncompatibleSave, true));
    TestTrue(TEXT("Rejected snapshot import preserves the active grid"),
        FMath::IsNearlyEqual(Excavation->CellSizeCm, 25.0f));
    TestEqual(TEXT("Rejected snapshot import preserves active chunk cells"),
        Excavation->CellsPerChunkAxis, 32);

    World->DestroyWorld(false);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
