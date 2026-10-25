import importlib.util
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "verify_test_logs.py"
SPEC = importlib.util.spec_from_file_location("verify_test_logs", SCRIPT)
VERIFY = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(VERIFY)


class LogVerifierTests(unittest.TestCase):
    def setUp(self):
        self.tempdir = tempfile.TemporaryDirectory()
        self.root = Path(self.tempdir.name)
        self.mechanics = self.root / "mechanics.log"
        self.wood = self.root / "wood.log"
        self.stick_inventory = self.root / "stick_inventory.log"
        self.stone_placement = self.root / "stone_placement.log"
        self.stone_trap = self.root / "stone_trap.log"
        self.inventory_capacity = self.root / "inventory_capacity.log"
        self.world_snap = self.root / "world_snap.log"
        self.stone_shape = self.root / "stone_shape.log"

    def tearDown(self):
        self.tempdir.cleanup()

    def test_valid_logs_pass(self):
        self.mechanics.write_text(
            "[MechanicsDemo] GEÇTİ test\nSONUÇ: 101 geçti, 0 kaldı\n",
            encoding="utf-8",
        )
        self.wood.write_text(
            "Test Completed. Result={Success} Name={A} Path={MyProject2.Wood.A}\n"
            "Test Completed. Result={Success} Name={B} Path={MyProject2.Wood.B}\n",
            encoding="utf-8",
        )
        self.assertEqual(VERIFY.verify_mechanics(self.mechanics), (101, 0))
        self.assertEqual(VERIFY.verify_wood(self.wood), (2, 0))

    def test_mechanics_failure_is_rejected(self):
        self.mechanics.write_text(
            "[MechanicsDemo] KALDI broken\nSONUÇ: 100 geçti, 1 kaldı\n",
            encoding="utf-8",
        )
        with self.assertRaisesRegex(ValueError, "1 failed"):
            VERIFY.verify_mechanics(self.mechanics)

    def test_missing_mechanics_summary_is_rejected(self):
        self.mechanics.write_text("crashed before summary\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "missing"):
            VERIFY.verify_mechanics(self.mechanics)

    def test_wood_failure_is_rejected(self):
        self.wood.write_text(
            "Test Completed. Result={Fail} Name={A} Path={MyProject2.Wood.A}\n",
            encoding="utf-8",
        )
        with self.assertRaisesRegex(ValueError, "1 failed"):
            VERIFY.verify_wood(self.wood)

    def test_missing_wood_results_are_rejected(self):
        self.wood.write_text("Automation tests did not run\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "no parseable"):
            VERIFY.verify_wood(self.wood)


    def test_fewer_than_required_wood_tests_are_rejected(self):
        self.wood.write_text(
            "Test Completed. Result={Success} Name={A} Path={MyProject2.Wood.A}\\n"
            "Test Completed. Result={Success} Name={B} Path={MyProject2.Wood.B}\\n",
            encoding="utf-8",
        )
        with self.assertRaisesRegex(ValueError, "expected at least 3"):
            VERIFY.verify_wood(self.wood, minimum_tests=3)

    def test_valid_stick_inventory_log_passes(self):
        self.stick_inventory.write_text(
            "Test Completed. Result={Success} Name={Preflight} "
            "Path={MyProject2.Inventory.StickCraftTransaction.Preflight}\n"
            "Test Completed. Result={Success} Name={Capacity} "
            "Path={MyProject2.Inventory.StickCraftTransaction.Capacity}\n"
            "Test Completed. Result={Success} Name={Success} "
            "Path={MyProject2.Inventory.StickCraftTransaction.Success}\n",
            encoding="utf-8",
        )
        self.assertEqual(VERIFY.verify_stick_inventory(self.stick_inventory), (3, 0))

    def test_stick_inventory_failure_is_rejected(self):
        self.stick_inventory.write_text(
            "Test Completed. Result={Success} Name={Preflight} "
            "Path={MyProject2.Inventory.StickCraftTransaction.Preflight}\n"
            "Test Completed. Result={Fail} Name={Capacity} "
            "Path={MyProject2.Inventory.StickCraftTransaction.Capacity}\n",
            encoding="utf-8",
        )
        with self.assertRaisesRegex(ValueError, "1 failed"):
            VERIFY.verify_stick_inventory(self.stick_inventory)

    def test_missing_stick_inventory_result_is_rejected(self):
        self.stick_inventory.write_text("Automation suite aborted\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "no parseable"):
            VERIFY.verify_stick_inventory(self.stick_inventory)

    def test_stick_inventory_requires_expected_suite_prefix(self):
        self.stick_inventory.write_text(
            "Test Completed. Result={Success} Name={Other} Path={MyProject2.Other.Test}\n",
            encoding="utf-8",
        )
        with self.assertRaisesRegex(ValueError, "no test result matched"):
            VERIFY.verify_stick_inventory(self.stick_inventory)

    def test_valid_stone_placement_log_passes(self):
        names = ("BeginCancel", "FaceAndSocket", "ConfirmGate", "CollisionClearance", "StructuralConnection")
        self.stone_placement.write_text(
            "".join(
                f"Test Completed. Result={{Success}} Name={{{name}}} "
                f"Path={{MyProject2.StonePlacement.Automation.{name}}}\\n"
                for name in names
            ),
            encoding="utf-8",
        )
        self.assertEqual(VERIFY.verify_stone_placement(self.stone_placement), (5, 0))

    def test_stone_placement_failure_is_rejected(self):
        self.stone_placement.write_text(
            "Test Completed. Result={Success} Name={BeginCancel} "
            "Path={MyProject2.StonePlacement.BeginCancel}\\n"
            "Test Completed. Result={Fail} Name={FaceAndSocket} "
            "Path={MyProject2.StonePlacement.FaceAndSocket}\\n",
            encoding="utf-8",
        )
        with self.assertRaisesRegex(ValueError, "1 failed"):
            VERIFY.verify_stone_placement(self.stone_placement)

    def test_missing_stone_placement_result_is_rejected(self):
        self.stone_placement.write_text("Automation suite aborted\\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "no parseable"):
            VERIFY.verify_stone_placement(self.stone_placement)

    def test_valid_stone_trap_log_passes(self):
        names = ("DispatchAndCooldown", "NullVictimSafety", "PhysicalStone", "CarvableLog")
        self.stone_trap.write_text(
            "".join(
                f"Test Completed. Result={{Success}} Name={{{name}}} "
                f"Path={{MyProject2.StoneTrap.Damage.{name}}}\\n"
                for name in names
            ),
            encoding="utf-8",
        )
        self.assertEqual(VERIFY.verify_stone_trap(self.stone_trap), (4, 0))

    def test_stone_trap_failure_is_rejected(self):
        self.stone_trap.write_text(
            "Test Completed. Result={Success} Name={Damage} "
            "Path={MyProject2.StoneTrap.Damage.DispatchAndCooldown}\\n"
            "Test Completed. Result={Fail} Name={Cooldown} "
            "Path={MyProject2.StoneTrap.Damage.NullVictimSafety}\\n",
            encoding="utf-8",
        )
        with self.assertRaisesRegex(ValueError, "1 failed"):
            VERIFY.verify_stone_trap(self.stone_trap)

    def test_missing_stone_trap_result_is_rejected(self):
        self.stone_trap.write_text("Automation suite aborted\\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "no parseable"):
            VERIFY.verify_stone_trap(self.stone_trap)

    def test_stone_trap_minimum_test_count_is_enforced(self):
        self.stone_trap.write_text(
            "Test Completed. Result={Success} Name={Damage} "
            "Path={MyProject2.StoneTrap.Damage.DispatchAndCooldown}\\n",
            encoding="utf-8",
        )
        with self.assertRaisesRegex(ValueError, "expected at least 4"):
            VERIFY.verify_stone_trap(self.stone_trap)

    def test_fewer_than_required_mechanics_checks_are_rejected(self):
        self.mechanics.write_text("SONUÇ: 100 geçti, 0 kaldı\\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "expected at least 101"):
            VERIFY.verify_mechanics(self.mechanics, minimum_passed=101)


    def test_valid_inventory_capacity_log_passes(self):
        self.inventory_capacity.write_text(
            "Test Completed. Result={Success} Name={ShrinkRollsBack} "
            "Path={MyProject2.Inventory.Core.Capacity.ShrinkRollsBack}\n"
            "Test Completed. Result={Success} Name={ValidAdd} "
            "Path={MyProject2.Inventory.Core.Capacity.ValidAdd}\n"
            "Test Completed. Result={Success} Name={RestoreFailurePreservesCurrent} "
            "Path={MyProject2.Inventory.Core.Capacity.RestoreFailurePreservesCurrent}\n"
            "Test Completed. Result={Success} Name={RestoreInvalidEntryPreservesCurrent} "
            "Path={MyProject2.Inventory.Core.Capacity.RestoreInvalidEntryPreservesCurrent}\n"
            "Test Completed. Result={Success} Name={InvalidMetadataRejected} "
            "Path={MyProject2.Inventory.Core.Capacity.InvalidMetadataRejected}\n",
            encoding="utf-8",
        )
        self.assertEqual(VERIFY.verify_inventory_capacity(self.inventory_capacity), (5, 0))

    def test_inventory_capacity_failure_is_rejected(self):
        self.inventory_capacity.write_text(
            "Test Completed. Result={Success} Name={ShrinkRollsBack} "
            "Path={MyProject2.Inventory.Core.Capacity.ShrinkRollsBack}\n"
            "Test Completed. Result={Fail} Name={ValidAdd} "
            "Path={MyProject2.Inventory.Core.Capacity.ValidAdd}\n",
            encoding="utf-8",
        )
        with self.assertRaisesRegex(ValueError, "1 failed"):
            VERIFY.verify_inventory_capacity(self.inventory_capacity)

    def test_missing_inventory_capacity_results_are_rejected(self):
        self.inventory_capacity.write_text("Automation suite aborted\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "no parseable"):
            VERIFY.verify_inventory_capacity(self.inventory_capacity)

    def test_inventory_capacity_minimum_test_count_is_enforced(self):
        self.inventory_capacity.write_text(
            "Test Completed. Result={Success} Name={ShrinkRollsBack} "
            "Path={MyProject2.Inventory.Core.Capacity.ShrinkRollsBack}\n",
            encoding="utf-8",
        )
        with self.assertRaisesRegex(ValueError, "expected at least 5"):
            VERIFY.verify_inventory_capacity(self.inventory_capacity)

    def test_valid_world_snap_log_passes(self):
        names = (
            "Resolver.NearestCompatibleCandidate",
            "Resolver.DeterministicTieBreak",
            "Resolver.RequiredGroupFilter",
            "Resolver.OccupiedCandidateSkipped",
            "Resolver.UnavailableCandidateSkipped",
            "Resolver.LoadBearingRequired",
            "TwoAnchor.LevelPlacement",
            "TwoAnchor.SlopePitch",
            "TwoAnchor.LengthMismatch",
            "TwoAnchor.NonLoadBearing",
            "TwoAnchor.SameAnchor",
            "TwoAnchor.PitchRange",
        )
        self.world_snap.write_text(
            "".join(
                f"Test Completed. Result={{Success}} Name={{{name}}} "
                f"Path={{WorldSnap.{name}}}\n"
                for name in names
            ),
            encoding="utf-8",
        )
        self.assertEqual(VERIFY.verify_world_snap(self.world_snap), (12, 0))

    def test_world_snap_failure_is_rejected(self):
        self.world_snap.write_text(
            "Test Completed. Result={Success} Name={Level} Path={WorldSnap.Level}\n"
            "Test Completed. Result={Fail} Name={Slope} Path={WorldSnap.Slope}\n",
            encoding="utf-8",
        )
        with self.assertRaisesRegex(ValueError, "1 failed"):
            VERIFY.verify_world_snap(self.world_snap)

    def test_missing_world_snap_results_are_rejected(self):
        self.world_snap.write_text("Automation suite aborted\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "no parseable"):
            VERIFY.verify_world_snap(self.world_snap)

    def test_valid_stone_shape_recognition_log_passes(self):
        names = (
            "RegularRing",
            "LineRejected",
            "UnevenGroundRejected",
            "PartialArcRejected",
        )
        self.stone_shape.write_text(
            "".join(
                f"Test Completed. Result={{Success}} Name={{{name}}} "
                f"Path={{MyProject2.Construction.StoneShapeRecognition.{name}}}\n"
                for name in names
            ),
            encoding="utf-8",
        )
        self.assertEqual(VERIFY.verify_stone_shape_recognition(self.stone_shape), (4, 0))

    def test_stone_shape_recognition_failure_is_rejected(self):
        self.stone_shape.write_text(
            "Test Completed. Result={Success} Name={Ring} "
            "Path={MyProject2.Construction.StoneShapeRecognition.RegularRing}\n"
            "Test Completed. Result={Fail} Name={Line} "
            "Path={MyProject2.Construction.StoneShapeRecognition.LineRejected}\n",
            encoding="utf-8",
        )
        with self.assertRaisesRegex(ValueError, "1 failed"):
            VERIFY.verify_stone_shape_recognition(self.stone_shape)

    def test_missing_stone_shape_recognition_results_are_rejected(self):
        self.stone_shape.write_text("Automation suite aborted\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "no parseable"):
            VERIFY.verify_stone_shape_recognition(self.stone_shape)

if __name__ == "__main__":
    unittest.main(verbosity=2)
