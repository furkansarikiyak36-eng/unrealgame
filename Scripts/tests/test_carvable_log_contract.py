#!/usr/bin/env python3
"""Static integration contracts for the existing ACarvableLog repair/support path."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
LOG_CPP = (ROOT / "Source/MyProject2/CarvableLog.cpp").read_text(encoding="utf-8")
MECHANICS = (ROOT / "Source/MyProject2/MechanicsDemo.cpp").read_text(encoding="utf-8")


def section(source: str, start: str, end: str) -> str:
    begin = source.index(start)
    finish = source.index(end, begin + len(start))
    return source[begin:finish]


class CarvableLogContractTests(unittest.TestCase):
    def test_replacement_requires_compatible_unmodified_stock(self):
        can_replace = section(LOG_CPP, "bool ACarvableLog::CanReplace", "bool ACarvableLog::ReplaceDamaged")
        # The player puts the replacement in from the shoulder: a carried piece must stay acceptable.
        self.assertNotIn("Replacement->IsCarried()", can_replace)
        self.assertIn("Replacement->MeshComponent->IsSimulatingPhysics()", can_replace)
        self.assertIn("Replacement->BurnState != EBurnState::Intact", can_replace)
        self.assertIn("Old->GetSplitCount() != Replacement->GetSplitCount()", can_replace)
        self.assertIn("Replacement->JointCuts.IsEmpty()", can_replace)
        self.assertIn("Replacement->CutSpans.IsEmpty()", can_replace)
        self.assertIn("FMath::IsNearlyEqual(Old->GetRadius(), Replacement->GetRadius(), 1.5f)", can_replace)
        self.assertIn("OldSize.Y, ReplacementSize.Y", can_replace)
        self.assertIn("OldSize.Z, ReplacementSize.Z", can_replace)

    def test_repair_rebuild_is_transactional_and_fails_closed(self):
        repair = section(LOG_CPP, "bool ACarvableLog::ReplaceDamaged", "bool ACarvableLog::Push")
        self.assertIn("BackupMesh->SetMesh", repair)
        self.assertIn("auto RollbackReplacement", repair)
        self.assertGreaterEqual(repair.count("RollbackReplacement();"), 2)
        self.assertIn("if (!bCutRebuilt)", repair)
        self.assertIn("default:                     bCutRebuilt = false;", repair)
        self.assertIn("Replacement->JointCuts = OriginalJointCuts;", repair)
        self.assertIn("Replacement->CutSpans = OriginalCutSpans;", repair)
        self.assertLess(repair.index("StructureLink::Settle(Old->GetWorld())"), repair.index("OnLogRepaired.Broadcast(Replacement)"))

    def test_damage_thresholds_re_evaluate_the_structure(self):
        damage = section(LOG_CPP, "void ACarvableLog::SetDamage", "float ACarvableLog::GetIgniteSeconds")
        self.assertIn("WoodStructure::JointLooseDamage", damage)
        self.assertIn("WoodStructure::FailureDamage", damage)
        self.assertIn("StructureLink::Settle(GetWorld())", damage)

    def test_installed_opening_keeps_both_remaining_pieces_installed(self):
        opening = section(LOG_CPP, "ACarvableLog* ACarvableLog::CutOpening", "bool ACarvableLog::CutGroove")
        self.assertIn("const bool bWasAnchored = bAnchored;", opening)
        self.assertIn("if (bWasAnchored)", opening)
        self.assertIn("Anchor();", opening)
        self.assertIn("Right->Anchor();", opening)
        self.assertIn("StructureLink::Settle(GetWorld())", opening)

    def test_existing_mechanics_demo_still_observes_repair_and_carving(self):
        self.assertIn("ACarvableLog::OnLogRepaired.AddUObject", MECHANICS)
        self.assertIn("ACarvableLog::OnLogCarved.AddUObject", MECHANICS)
        self.assertIn("StructureLink::Settle(World)", MECHANICS)


if __name__ == "__main__":
    unittest.main(verbosity=2)
