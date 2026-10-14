#!/usr/bin/env python3
"""Static contracts for Phase 8 shared world-state simulation and persistence."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORLD_H = (ROOT / "Source/MyProject2/WorldCore/WorldStateSubsystem.h").read_text(encoding="utf-8")
WORLD_CPP = (ROOT / "Source/MyProject2/WorldCore/WorldStateSubsystem.cpp").read_text(encoding="utf-8")
CHUNK_H = (ROOT / "Source/MyProject2/WorldCore/ChunkState.h").read_text(encoding="utf-8")
STORE_CPP = (ROOT / "Source/MyProject2/WorldCore/ChunkStateStore.cpp").read_text(encoding="utf-8")
TYPES_H = (ROOT / "Source/MyProject2/WorldCore/WorldTypes.h").read_text(encoding="utf-8")
SAVE_H = (ROOT / "Source/MyProject2/WorldCore/WorldPersistenceSubsystem.h").read_text(encoding="utf-8")
SAVE_CPP = (ROOT / "Source/MyProject2/WorldCore/WorldPersistenceSubsystem.cpp").read_text(encoding="utf-8")
SIM_STATE_H = (ROOT / "Source/MyProject2/SimCore/SimulationState.h").read_text(encoding="utf-8")
SIM_CPP = (ROOT / "Source/MyProject2/SimCore/SimulationSubsystem.cpp").read_text(encoding="utf-8")
FIRE_CPP = (ROOT / "Source/MyProject2/WorldPhaseW5_FireDecayChem/FireSimulation.cpp").read_text(encoding="utf-8")
RUNNER = (ROOT / "Scripts/run_python_tests.sh").read_text(encoding="utf-8")


def section(source: str, start: str, end: str) -> str:
    begin = source.index(start)
    finish = source.index(end, begin + len(start))
    return source[begin:finish]


class WorldStateIntegrationTests(unittest.TestCase):
    def test_chunk_cells_and_metadata_are_save_serializable(self):
        self.assertIn("struct FChunkStateSnapshot", CHUNK_H)
        self.assertIn("struct FWorldStateSnapshot", CHUNK_H)
        self.assertIn("UPROPERTY(SaveGame, BlueprintReadWrite) FWaterCell Water;", CHUNK_H)
        self.assertIn("UPROPERTY(SaveGame, BlueprintReadWrite) FBiomassCell Biomass;", CHUNK_H)
        self.assertIn("LastObservedWorldDay", CHUNK_H)
        for field in ("SoilMoisture", "SoilNitrogen", "SoilPhosphorus", "SoilPotassium", "SoilPH"):
            self.assertIn(f"UPROPERTY(SaveGame, BlueprintReadWrite) float {field}", CHUNK_H)
        self.assertIn("UPROPERTY(SaveGame, BlueprintReadWrite) float Volume", TYPES_H)

    def test_world_snapshot_validates_then_replaces_the_canonical_chunk_store(self):
        self.assertIn("bool ExportSnapshot(FWorldStateSnapshot& OutSnapshot) const", WORLD_H)
        self.assertIn("bool ValidateSnapshot(const FWorldStateSnapshot& Snapshot) const", WORLD_H)
        self.assertIn("bool ImportSnapshot(const FWorldStateSnapshot& Snapshot)", WORLD_H)
        self.assertIn("if(!StateStore || !ValidateSnapshot(Snapshot)", WORLD_CPP)
        self.assertIn("ReplaceAllChunks(Snapshot.Chunks,Snapshot.CellsPerChunkAxis)", WORLD_CPP)
        self.assertIn("Chunks = MoveTemp(NewChunks)", STORE_CPP)
        self.assertIn("MaxSnapshotCells = 1048576", WORLD_H)

    def test_schema_four_saves_world_state_and_keeps_legacy_schemas_readable(self):
        self.assertIn("FWorldStateSnapshot WorldState;", SAVE_H)
        self.assertIn("bool bHasWorldState = false;", SAVE_H)
        self.assertIn("CurrentSaveSchemaVersion = 4", SAVE_H)
        self.assertIn("Manifest.SchemaVersion != 3", SAVE_CPP)
        self.assertIn("WorldState->ExportSnapshot(WorldStateSnapshot)", SAVE_CPP)
        self.assertIn("WorldState->ValidateSnapshot(LoadedSave->WorldState)", SAVE_CPP)
        self.assertIn("WorldState->ImportSnapshot(LoadedSave->WorldState)", SAVE_CPP)

    def test_domain_simulations_share_a_single_ordered_chunk_pipeline(self):
        pipeline = section(WORLD_CPP, "void UWorldStateSubsystem::SimulateChunk", "void UWorldStateSubsystem::ApplyCrossDomainCoupling")
        ordered = (
            "WaterSimulation->SimulateChunk",
            "SoilSimulation->SimulateChunk",
            "ChemistrySimulation->SimulateChunk",
            "ApplyCrossDomainCoupling",
            "FireSimulation->SimulateChunk",
            "DecaySimulation->SimulateChunk",
        )
        positions = [pipeline.index(token) for token in ordered]
        self.assertEqual(positions, sorted(positions))
        self.assertIn("Cell.SoilMoisture", WORLD_CPP)
        self.assertIn("Biomass.MoistureContent=NewMoisture", WORLD_CPP)
        self.assertIn("Toxin*ToxinRate*Days", WORLD_CPP)

    def test_shared_weather_drives_deterministic_wind_and_wet_biomass_resists_fire(self):
        self.assertIn("FVector2D WindDirection", SIM_STATE_H)
        self.assertIn("WindAngle=DailyRng.Range", SIM_CPP)
        self.assertIn("WindDirection=FVector2D", SIM_CPP)
        self.assertIn("WindDirection = SimState->WindDirection", WORLD_CPP)
        self.assertIn("FireSimulation->SimulateChunk(Chunk,StepDays,WindSpeed,WindDirection)", WORLD_CPP)
        self.assertIn("X.Biomass.MoistureContent>0.7f", FIRE_CPP)
        self.assertIn("A.Biomass.MoistureContent>0.7f", FIRE_CPP)

    def test_lazy_catch_up_is_budgeted_and_rain_history_is_capped(self):
        self.assertIn("AccumulatedDeltaDays", WORLD_CPP)
        self.assertIn("LastObservedWorldDay", WORLD_CPP)
        self.assertIn("MaxSimulationDaysPerStep=1.0f", WORLD_H)
        self.assertIn("MaxAccumulatedSimulationDays=365.0f", WORLD_H)
        self.assertIn("MaxRainfallDaysToReconstruct = 365", WORLD_H)
        self.assertIn("CurrentDayIndex - MaxRainfallDaysToReconstruct + 1", WORLD_CPP)
        self.assertIn("MaxChunksSimulatedPerFrame", WORLD_CPP)

    def test_phase8_contract_is_wired_into_python_ci(self):
        self.assertIn("test_phase8_world_state_integration_contract.py", RUNNER)


if __name__ == "__main__":
    unittest.main(verbosity=2)
