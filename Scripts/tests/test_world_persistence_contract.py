#!/usr/bin/env python3
"""Static contract checks for Phase 2 world identity, idempotency, and save envelope."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "Source/MyProject2/WorldCore/WorldPersistenceSubsystem.h").read_text(encoding="utf-8")
SOURCE = (ROOT / "Source/MyProject2/WorldCore/WorldPersistenceSubsystem.cpp").read_text(encoding="utf-8")


class WorldPersistenceContractTests(unittest.TestCase):
    def test_versioned_save_envelope_contains_canonical_identity_and_excavation_snapshot(self):
        self.assertIn("class MYPROJECT2_API UWorldCoreSaveGame : public USaveGame", HEADER)
        for marker in ("SchemaVersion", "WorldId", "WorldSeed", "WorldRevision", "FExcavationVolumeSnapshot Excavation"):
            self.assertIn(marker, HEADER)
        self.assertIn("PROPERTY(SaveGame", HEADER)

    def test_commands_are_idempotent_and_invalid_ids_fail_closed(self):
        for method in ("ApplySphereEditOnce", "ApplyCapsuleEditOnce", "ApplyBoxEditOnce"):
            self.assertIn(method, HEADER)
            self.assertIn("ProcessedCommandIdSet.Contains(CommandId)", SOURCE)
        self.assertIn("EWorldCoreCommandResult::InvalidCommand", SOURCE)
        self.assertIn("EWorldCoreCommandResult::AlreadyApplied", SOURCE)

    def test_save_load_uses_the_authoritative_excavation_subsystem(self):
        self.assertIn("Excavation->ExportSnapshot(Snapshot)", SOURCE)
        self.assertIn("ImportSnapshot(LoadedSave->Excavation, true)", SOURCE)
        self.assertIn("UGameplayStatics::SaveGameToSlot", SOURCE)
        self.assertIn("UGameplayStatics::LoadGameFromSlot", SOURCE)

    def test_load_suppresses_synthetic_revision_and_validates_ledgers_before_import(self):
        self.assertIn("bApplyingLoadedState = true", SOURCE)
        self.assertIn("bApplyingLoadedState = false", SOURCE)
        self.assertLess(SOURCE.find("NewProcessedCommandIdSet.Contains(CommandId)"), SOURCE.find("ImportSnapshot(LoadedSave->Excavation, true)"))
        self.assertLess(SOURCE.find("NewRecentWorldEventKeySet.Contains(EventKey)"), SOURCE.find("ImportSnapshot(LoadedSave->Excavation, true)"))

    def test_commit_and_rollback_have_distinct_deduplicated_event_keys(self):
        self.assertIn('bIsRollback ? TEXT(":R") : TEXT(":C")', SOURCE)
        self.assertIn("RecentWorldEventKeySet.Contains(EventKey)", SOURCE)


if __name__ == "__main__":
    unittest.main(verbosity=2)
