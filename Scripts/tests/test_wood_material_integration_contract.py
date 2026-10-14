#!/usr/bin/env python3
"""Static contract for Phase 7's one-owner wood condition/structure integration."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
LOG_H = (ROOT / "Source/MyProject2/CarvableLog.h").read_text(encoding="utf-8")
LOG_CPP = (ROOT / "Source/MyProject2/CarvableLog.cpp").read_text(encoding="utf-8")
WOOD_H = (ROOT / "Source/MyProject2/WorldCore/WoodMaterialSubsystem.h").read_text(encoding="utf-8")
WOOD_CPP = (ROOT / "Source/MyProject2/WorldCore/WoodMaterialSubsystem.cpp").read_text(encoding="utf-8")
BURN_H = (ROOT / "Source/MyProject2/BurnSubsystem.h").read_text(encoding="utf-8")
BURN_CPP = (ROOT / "Source/MyProject2/BurnSubsystem.cpp").read_text(encoding="utf-8")
STRUCTURE_H = (ROOT / "Source/MyProject2/StructureLink.h").read_text(encoding="utf-8")
STRUCTURE_CPP = (ROOT / "Source/MyProject2/StructureLink.cpp").read_text(encoding="utf-8")
RUNNER = (ROOT / "Scripts/run_python_tests.sh").read_text(encoding="utf-8")


def section(source: str, start: str, end: str) -> str:
    begin = source.index(start)
    finish = source.index(end, begin + len(start))
    return source[begin:finish]


class WoodMaterialIntegrationTests(unittest.TestCase):
    def test_existing_damage_remains_the_single_structural_integrity_value(self):
        self.assertIn("float Damage = 0.f;", LOG_H)
        self.assertIn("void ACarvableLog::SetDamage(float Value)", LOG_CPP)
        damage = section(LOG_CPP, "void ACarvableLog::SetDamage", "void ACarvableLog::SimulateMaterialEnvironment")
        self.assertIn("WoodStructure::JointLooseDamage", damage)
        self.assertIn("WoodStructure::FailureDamage", damage)
        self.assertIn("StructureLink::Settle(GetWorld())", damage)
        self.assertIn("AddDamage(EnvironmentalDamage)", LOG_CPP)

    def test_material_condition_is_separate_from_fire_state_and_bounded(self):
        self.assertIn("float MoistureContent = 0.0f;", LOG_H)
        self.assertIn("float MaterialAgeDays = 0.0f;", LOG_H)
        env = section(LOG_CPP, "void ACarvableLog::SimulateMaterialEnvironment", "float ACarvableLog::GetIgniteSeconds")
        self.assertIn("FMath::IsFinite(ElapsedDays)", env)
        self.assertIn("FMath::Clamp(MoistureContent + AddedMoisture, 0.0f, 1.0f)", env)
        self.assertIn("FMath::Min(ElapsedDays, 365.0f)", env)
        self.assertIn("MaterialDefinition->Properties", env)
        self.assertIn("BurnState == EBurnState::Charred", env)

    def test_moisture_changes_fire_resistance_without_adding_a_second_fire_owner(self):
        ignite = section(LOG_CPP, "float ACarvableLog::GetIgniteSeconds", "float ACarvableLog::GetHeatShare")
        self.assertIn("MoistureContent", ignite)
        self.assertIn("BurnTotal", LOG_CPP)
        self.assertIn("UBurnSubsystem", LOG_CPP)
        self.assertIn("TArray<TWeakObjectPtr<ACarvableLog>> Burning", BURN_H)
        self.assertIn("Source->StepBurn(Seconds)", BURN_CPP)
        self.assertNotIn("EBurnState", WOOD_H)
        self.assertNotIn("EBurnState", WOOD_CPP)

    def test_weather_adapter_uses_shared_world_clock_and_bounded_actor_budget(self):
        self.assertIn("Collection.InitializeDependency<UWorldStateSubsystem>()", WOOD_CPP)
        self.assertIn("WorldState->GetWorldTime().ToTotalDays()", WOOD_CPP)
        self.assertIn("MaxLogsProcessedPerTick", WOOD_H)
        self.assertIn("while (BatchCursor < TrackedLogs.Num() && ProcessedThisTick < Budget)", WOOD_CPP)
        self.assertIn("TrackedLogs.Sort", WOOD_CPP)
        self.assertIn("Log->SimulateMaterialEnvironment", WOOD_CPP)
        self.assertIn("IsExposedToRain(Log)", WOOD_CPP)

    def test_structure_graph_stays_in_structure_link_and_not_in_the_material_adapter(self):
        self.assertIn("WoodStructure::FNode MakeNode", STRUCTURE_H)
        self.assertIn("WoodStructure::ComputeSupport", STRUCTURE_CPP)
        self.assertIn("int32 Settle(UWorld* World)", STRUCTURE_H)
        self.assertNotIn("WoodStructure::FNode", WOOD_H)
        self.assertNotIn("WoodStructure::FNode", WOOD_CPP)
        self.assertIn("StructureLink::Settle(GetWorld())", LOG_CPP)

    def test_phase7_contract_is_part_of_python_ci(self):
        self.assertIn("test_wood_material_integration_contract.py", RUNNER)


if __name__ == "__main__":
    unittest.main(verbosity=2)
