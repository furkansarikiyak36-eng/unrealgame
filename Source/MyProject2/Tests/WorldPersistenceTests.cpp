// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "WorldCore/ExcavationVolumeSubsystem.h"
#include "Shoring/ShoringTypes.h"
#include "SurvivalComponent.h"
#include "Phase7_Integration/SaveGameSubsystem.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StoneCore/StoneDefinition.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

namespace
{
	UWorld* CreateWorldPersistenceTestWorld(const TCHAR* Name)
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

	FShoringPieceSaveData MakeValidShoringSaveRecord()
	{
		FShoringPieceSaveData Record;
		Record.Spec.PieceId = FGuid::NewGuid();
		Record.Spec.PlanId = FGuid::NewGuid();
		Record.Spec.Kind = EShoringPieceKind::Upright;
		Record.Spec.Transform = FTransform::Identity;
		Record.Spec.HalfExtentsCm = FVector(10.0f);
		Record.Spec.DesignLoad = 1.0f;
		Record.Spec.SupportAnchor.AnchorId = FGuid::NewGuid();
		Record.Spec.SupportAnchor.Kind = EShoringAnchorKind::Ground;
		Record.Spec.SupportAnchor.PositionCm = FVector::ZeroVector;
		Record.Spec.SupportAnchor.Normal = FVector::UpVector;
		Record.Spec.SupportAnchor.SourceDensityRevision = 1;
		Record.SavedState = EShoringPieceState::Active;
		Record.Health = 100.0f;
		Record.MaxHealth = 100.0f;
		return Record;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldPersistenceVoxelSnapshotRoundTripTest,
	"MyProject2.World.Persistence.VoxelSnapshotRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldPersistenceVoxelSnapshotRoundTripTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateWorldPersistenceTestWorld(TEXT("WorldPersistenceVoxelSnapshotWorld"));
	if (!TestNotNull(TEXT("World should be created"), World))
	{
		return false;
	}
	UExcavationVolumeSubsystem* Volume = World->GetSubsystem<UExcavationVolumeSubsystem>();
	if (!TestNotNull(TEXT("Volume subsystem should exist"), Volume))
	{
		World->DestroyWorld(false);
		return false;
	}

	FString Error;
	TestTrue(TEXT("Grid should configure before edits"), Volume->ConfigureGridForRuntimeProfile(25.0f, 32, Error));
	FExcavationEditRecord Edit;
	const FVector Center = Volume->CellToWorldCenter(FIntVector(16, 16, -16));
	TestTrue(TEXT("A sphere carve should produce persistent density overrides"),
		Volume->ApplySphereEdit(Center, 40.0f, EExcavationEditType::Carve, Edit));

	FExcavationVolumeSnapshot Snapshot;
	TestTrue(TEXT("Voxel state should export"), Volume->ExportSnapshot(Snapshot));
	const int32 SavedCount = Snapshot.Overrides.Num();
	TestTrue(TEXT("Snapshot should contain modified cells"), SavedCount > 0);
	TestTrue(TEXT("Valid snapshot should pass preflight validation"), Volume->ValidateImportSnapshot(Snapshot, true, Error));

	FExcavationVolumeSnapshot DuplicateCell = Snapshot;
	if (DuplicateCell.Overrides.Num() > 0)
	{
		DuplicateCell.Overrides.Add(DuplicateCell.Overrides[0]);
		TestFalse(TEXT("Duplicate cell entries must fail preflight"), Volume->ValidateImportSnapshot(DuplicateCell, true, Error));
	}

	FExcavationEditRecord SecondEdit;
	TestTrue(TEXT("A second edit should change the live data"),
		Volume->ApplySphereEdit(Center + FVector(150.0f, 0.0f, 0.0f), 30.0f, EExcavationEditType::Carve, SecondEdit));
	TestTrue(TEXT("Import should restore the saved sparse density state"), Volume->ImportSnapshot(Snapshot, true));
	TestEqual(TEXT("Restored override count should match the snapshot"), Volume->GetOverrideCellCount(), SavedCount);

	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldPersistenceShoringSnapshotValidationTest,
	"MyProject2.World.Persistence.ShoringSnapshotRejectsDuplicateIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldPersistenceShoringSnapshotValidationTest::RunTest(const FString& Parameters)
{
	FShoringWorldSnapshot Snapshot;
	Snapshot.Pieces.Add(MakeValidShoringSaveRecord());
	FString Error;
	TestTrue(TEXT("A valid shoring snapshot should pass validation"), Snapshot.Validate(Error));

	FShoringWorldSnapshot Duplicate = Snapshot;
	Duplicate.Pieces.Add(Duplicate.Pieces[0]);
	TestFalse(TEXT("Duplicate member GUIDs must fail validation"), Duplicate.Validate(Error));
	TestFalse(TEXT("Invalid snapshot should provide a diagnostic"), Error.IsEmpty());

	FShoringWorldSnapshot InvalidState = Snapshot;
	InvalidState.Pieces[0].Health = 0.0f;
	TestFalse(TEXT("A failed/zero-health mismatch must fail validation"), InvalidState.Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldPersistencePlayerSnapshotValidationTest,
	"MyProject2.World.Persistence.PlayerSnapshotRejectsInvalidStats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldPersistencePlayerSnapshotValidationTest::RunTest(const FString& Parameters)
{
	FSurvivalComponentSnapshot Snapshot;
	FString Error;
	TestTrue(TEXT("Default player survival snapshot should be valid"), Snapshot.Validate(Error));

	Snapshot.Stamina = Snapshot.MaxValue + 1.0f;
	TestFalse(TEXT("Stamina above max must fail validation"), Snapshot.Validate(Error));

	Snapshot = FSurvivalComponentSnapshot();
	Snapshot.Health = 0.0f;
	TestFalse(TEXT("Zero health must agree with the saved dead flag"), Snapshot.Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldPersistenceStoneRecordValidationTest,
    "MyProject2.World.Persistence.StoneRecordsValidatePersistentStates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldPersistenceStoneRecordValidationTest::RunTest(const FString& Parameters)
{
    FStoneSaveData Record;
    Record.Definition = NewObject<UStoneDefinition>();
    Record.ActorClass = AStoneActor::StaticClass();
    Record.Transform = FTransform::Identity;
    Record.Health = 100.0f;
    Record.State = static_cast<uint8>(EStructuralNodeState::Loose);
    Record.Shape = static_cast<uint8>(EStoneShape::Building);
    Record.Dimensions = FVector(30.0f, 20.0f, 20.0f);
    FString Error;
    TestTrue(TEXT("Valid loose stone record passes validation"), Record.Validate(Error));
    Record.State = static_cast<uint8>(EStructuralNodeState::Carried);
    TestFalse(TEXT("Transient carried stones are rejected"), Record.Validate(Error));
    Record.State = static_cast<uint8>(EStructuralNodeState::Installed);
    Record.Health = 0.0f;
    TestFalse(TEXT("Zero-health stone records are rejected"), Record.Validate(Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldPersistenceVersionedSaveCompatibilityTest,
	"MyProject2.World.Persistence.VersionedSaveSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldPersistenceVersionedSaveCompatibilityTest::RunTest(const FString& Parameters)
{
	UWorldSaveGame* CompleteSave = NewObject<UWorldSaveGame>();
	if (!TestNotNull(TEXT("Versioned full-world save should be constructible"), CompleteSave))
	{
		return false;
	}
	TestEqual(TEXT("Current full-world save schema is version 3"), CompleteSave->SaveSchemaVersion, 3);
	TestNotNull(TEXT("Full-world save remains readable through the legacy log-save base class"),
		Cast<ULogSaveGame>(CompleteSave));

	ULogSaveGame* LegacySave = NewObject<ULogSaveGame>();
	TestNotNull(TEXT("Legacy log-only save remains constructible"), LegacySave);
	TestNull(TEXT("Legacy log-only object must not be misclassified as the full-world schema"),
		Cast<UWorldSaveGame>(LegacySave));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
