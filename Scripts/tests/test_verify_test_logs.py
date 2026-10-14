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

    def test_fewer_than_required_mechanics_checks_are_rejected(self):
        self.mechanics.write_text("SONUÇ: 100 geçti, 0 kaldı\\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "expected at least 101"):
            VERIFY.verify_mechanics(self.mechanics, minimum_passed=101)


if __name__ == "__main__":
    unittest.main(verbosity=2)
