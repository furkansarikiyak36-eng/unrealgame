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


if __name__ == "__main__":
    unittest.main(verbosity=2)
