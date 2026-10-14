#!/usr/bin/env python3
"""Static contract checks for Phase 4 underground support, collapse, water and trap layers."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORLD = ROOT / "Source" / "MyProject2" / "WorldCore"
HAZARD_H = (WORLD / "UndergroundHazardSubsystem.h").read_text(encoding="utf-8")
HAZARD_CPP = (WORLD / "UndergroundHazardSubsystem.cpp").read_text(encoding="utf-8")
SUPPORT_H = (WORLD / "UndergroundSupportActor.h").read_text(encoding="utf-8")
SUPPORT_CPP = (WORLD / "UndergroundSupportActor.cpp").read_text(encoding="utf-8")
BUILD_NODE_CPP = (ROOT / "Source" / "MyProject2" / "BuildingNodeActor.cpp").read_text(encoding="utf-8")
TYPES = (WORLD / "UndergroundHazardTypes.h").read_text(encoding="utf-8")
TRAP_H = (WORLD / "UndergroundTrapActor.h").read_text(encoding="utf-8")
TRAP_CPP = (WORLD / "UndergroundTrapActor.cpp").read_text(encoding="utf-8")
PERSIST_H = (WORLD / "WorldPersistenceSubsystem.h").read_text(encoding="utf-8")
PERSIST_CPP = (WORLD / "WorldPersistenceSubsystem.cpp").read_text(encoding="utf-8")
BASE_BUILD = (ROOT / "Source" / "MyProject2" / "MyProject2.Build.cs").read_text(encoding="utf-8")


class UndergroundHazardContractTests(unittest.TestCase):
    def test_support_actor_reuses_existing_structural_node_graph(self):
        self.assertIn("class MYPROJECT2_API AUndergroundSupportActor : public ABuildingNodeActor", SUPPORT_H)
        self.assertIn("UStructuralIntegritySubsystem", BUILD_NODE_CPP)
        self.assertIn("Super::BeginPlay()", SUPPORT_CPP)
        self.assertIn("ApplySupportProfile()", SUPPORT_CPP)
        self.assertIn("CompressiveCapacityKN", SUPPORT_CPP)
        self.assertIn("ApplyMoistureExposure", SUPPORT_H)
        self.assertIn("TriggerChaosCollapse", SUPPORT_CPP)

    def test_support_placement_is_idempotent_and_requires_cavity_clearance(self):
        self.assertIn("PlaceSupport(", HAZARD_H)
        self.assertIn("RegisteredSupports.Find(SupportId)", HAZARD_CPP)
        self.assertIn("SupportId.IsValid()", HAZARD_CPP)
        self.assertIn("BodyDensity >= 0.5f || HeadDensity >= 0.5f", HAZARD_CPP)
        self.assertIn("SpawnActorDeferred<AUndergroundSupportActor>", HAZARD_CPP)

    def test_wide_roof_spans_are_evaluated_with_support_protection_and_bounded_collapse(self):
        self.assertIn("ComputeCavitySpan", HAZARD_CPP)
        self.assertIn("CriticalUnsupportedRoofSpanCells", HAZARD_CPP)
        self.assertIn("HasOperationalSupportNear(RoofWorld", HAZARD_CPP)
        self.assertIn("MaxCollapseCellsPerTick", HAZARD_H)
        self.assertIn("ApplySphereEdit(", HAZARD_CPP)
        self.assertIn("bResolvingCollapse", HAZARD_CPP)
        self.assertIn("OnRoofCollapse.Broadcast", HAZARD_CPP)

    def test_large_edit_hazard_work_uses_bounded_reconciliation_and_stability_queues(self):
        self.assertIn("MaxVolumeReconciliationChecksPerTick", HAZARD_H)
        self.assertIn("PendingVolumeReconciliations", HAZARD_H)
        self.assertIn("ReconcileBudget", HAZARD_CPP)
        self.assertIn("MaxStabilityChecksPerTick", HAZARD_H)
        self.assertIn("MaxTrackedOpenCaveCells", HAZARD_H)

    def test_seepage_transfers_from_existing_surface_water_or_soil_into_3d_pockets(self):
        self.assertIn("UWorldStateSubsystem", HAZARD_CPP)
        self.assertIn("SurfaceCell->Water.Volume", HAZARD_CPP)
        self.assertIn("SurfaceCell->SoilMoisture", HAZARD_CPP)
        self.assertIn("FUndergroundWaterPocket& Pocket = WaterPockets.FindOrAdd(Cell)", HAZARD_CPP)
        self.assertIn("SimulateUndergroundGravityFlow", HAZARD_CPP)
        self.assertIn("SurfaceCellsProcessedThisTick.Contains(GlobalCell)", HAZARD_CPP)
        self.assertIn("OnWaterSeepage.Broadcast", HAZARD_CPP)

    def test_support_and_water_pocket_snapshots_are_versioned_and_validated(self):
        self.assertIn("FUndergroundSupportSaveRecord", TYPES)
        self.assertIn("FUndergroundSurfaceWaterCellSnapshot", TYPES)
        self.assertIn("FUndergroundHazardSnapshot", TYPES)
        self.assertIn("MaximumIntegrity", TYPES)
        self.assertIn("ValidateSnapshot", HAZARD_CPP)
        self.assertIn("Candidate.Supports.Sort", HAZARD_CPP)
        self.assertIn("Candidate.WaterPockets.Sort", HAZARD_CPP)

    def test_shared_save_envelope_persists_underground_state_and_tracks_dirty_changes(self):
        self.assertIn("FUndergroundHazardSnapshot UndergroundHazards", PERSIST_H)
        self.assertIn("CurrentSaveSchemaVersion = 5", PERSIST_H)
        self.assertIn("Hazards->ExportSnapshot(HazardSnapshot)", PERSIST_CPP)
        self.assertIn("Hazards->ImportSnapshot(LoadedSave->UndergroundHazards)", PERSIST_CPP)
        self.assertIn("HandleUndergroundWaterSeepage", PERSIST_CPP)
        self.assertIn("HandleUndergroundSupportChanged", PERSIST_CPP)
        self.assertIn("Manifest.SchemaVersion != 1 && Manifest.SchemaVersion != 2 && Manifest.SchemaVersion != 3 && Manifest.SchemaVersion != 4 && Manifest.SchemaVersion != CurrentSaveSchemaVersion", PERSIST_CPP)

    def test_underground_trap_reuses_stone_trap_damage_but_requires_open_cavity(self):
        self.assertIn("class MYPROJECT2_API AUndergroundTrapActor : public AStoneTrapActor", TRAP_H)
        self.assertIn("virtual void Trigger(AActor* Victim) override", TRAP_H)
        self.assertIn("HasValidCavityClearance()", TRAP_CPP)
        self.assertIn("Super::Trigger(Victim)", TRAP_CPP)
        self.assertIn("TriggerFromWaterPocket", TRAP_H)
        self.assertIn("TryGetWaterPocketAtCell", TRAP_CPP)


if __name__ == "__main__":
    unittest.main(verbosity=2)
