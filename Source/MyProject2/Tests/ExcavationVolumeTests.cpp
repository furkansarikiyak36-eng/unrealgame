#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "WorldCore/ExcavationVolumeSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExcavationVolumeEditRollbackIsOwnershipSafe,
    "MyProject2.Voxel.DataCore.RollbackRespectsNewerEditOwner",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FExcavationVolumeEditRollbackIsOwnershipSafe::RunTest(const FString& Parameters)
{
    UExcavationVolumeSubsystem* Volume = NewObject<UExcavationVolumeSubsystem>(GetTransientPackage());
    TestNotNull(TEXT("Volume subsystem object created"), Volume);
    if (!Volume) return false;

    Volume->CellSizeCm = 25.0f;
    Volume->CellsPerChunkAxis = 32;
    Volume->MaxCellsPerEdit = 250000;
    Volume->MaxSnapshotOverrideCells = 2000000;
    Volume->DefaultBaseDensity = 0.0f;

    FExcavationEditRecord EmptyCarve;
    TestFalse(TEXT("Carving default-empty density is a no-op"),
        Volume->ApplySphereEdit(FVector::ZeroVector, 50.0f, EExcavationEditType::Carve, EmptyCarve));

    FExcavationEditRecord FillRecord;
    TestTrue(TEXT("Fill creates overrides"),
        Volume->ApplySphereEdit(FVector::ZeroVector, 50.0f, EExcavationEditType::Fill, FillRecord));
    TestTrue(TEXT("Fill transaction is committed"), FillRecord.bCommitted);

    float Density = 0.0f;
    TestTrue(TEXT("Density query succeeds after fill"), Volume->TrySampleDensityAtWorld(FVector::ZeroVector, Density));
    TestTrue(TEXT("Filled cell is solid"), Density >= 0.99f);

    FExcavationVolumeSnapshot Snapshot;
    TestTrue(TEXT("Snapshot exports"), Volume->ExportSnapshot(Snapshot));
    TestTrue(TEXT("Snapshot contains sparse overrides"), Snapshot.Overrides.Num() > 0);

    FExcavationEditRecord CarveRecord;
    TestTrue(TEXT("Carve supersedes the fill edit"),
        Volume->ApplySphereEdit(FVector::ZeroVector, 50.0f, EExcavationEditType::Carve, CarveRecord));
    TestFalse(TEXT("Old fill cannot roll back a newer owner"), Volume->RollbackEdit(FillRecord));

    TestTrue(TEXT("Newest carve can roll back"), Volume->RollbackEdit(CarveRecord));
    TestTrue(TEXT("Density query succeeds after rollback"), Volume->TrySampleDensityAtWorld(FVector::ZeroVector, Density));
    TestTrue(TEXT("Rollback restores filled density"), Density >= 0.99f);

    TestTrue(TEXT("Original fill can now roll back"), Volume->RollbackEdit(FillRecord));
    TestTrue(TEXT("Density query succeeds after full rollback"), Volume->TrySampleDensityAtWorld(FVector::ZeroVector, Density));
    TestTrue(TEXT("Full rollback restores base-empty density"), Density <= 0.01f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FExcavationVolumeSnapshotRoundTrip,
    "MyProject2.Voxel.DataCore.SnapshotRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FExcavationVolumeSnapshotRoundTrip::RunTest(const FString& Parameters)
{
    UExcavationVolumeSubsystem* Source = NewObject<UExcavationVolumeSubsystem>(GetTransientPackage());
    UExcavationVolumeSubsystem* Destination = NewObject<UExcavationVolumeSubsystem>(GetTransientPackage());
    TestNotNull(TEXT("Source subsystem created"), Source);
    TestNotNull(TEXT("Destination subsystem created"), Destination);
    if (!Source || !Destination) return false;

    Source->CellSizeCm = 25.0f;
    Source->CellsPerChunkAxis = 32;
    Source->MaxCellsPerEdit = 250000;
    Source->MaxSnapshotOverrideCells = 2000000;
    Source->DefaultBaseDensity = 0.0f;

    FExcavationEditRecord FillRecord;
    TestTrue(TEXT("Source edit succeeds"),
        Source->ApplySphereEdit(FVector(200.0f, -75.0f, 50.0f), 50.0f, EExcavationEditType::Fill, FillRecord));

    FExcavationVolumeSnapshot Snapshot;
    TestTrue(TEXT("Snapshot export succeeds"), Source->ExportSnapshot(Snapshot));

    Destination->CellSizeCm = 25.0f;
    Destination->CellsPerChunkAxis = 32;
    Destination->MaxCellsPerEdit = 250000;
    Destination->MaxSnapshotOverrideCells = 2000000;
    Destination->DefaultBaseDensity = 0.0f;
    TestTrue(TEXT("Snapshot imports atomically"), Destination->ImportSnapshot(Snapshot, true));

    float SourceDensity = 0.0f;
    float DestinationDensity = 0.0f;
    const FVector TestPoint(200.0f, -75.0f, 50.0f);
    TestTrue(TEXT("Source density query"), Source->TrySampleDensityAtWorld(TestPoint, SourceDensity));
    TestTrue(TEXT("Destination density query"), Destination->TrySampleDensityAtWorld(TestPoint, DestinationDensity));
    TestTrue(TEXT("Source and destination logical density match"),
        FMath::IsNearlyEqual(SourceDensity, DestinationDensity, 0.0001f));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
