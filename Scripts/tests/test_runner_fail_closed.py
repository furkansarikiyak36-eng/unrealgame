#!/usr/bin/env python3
"""Static guard tests for the Windows UE test runner's report-update gate."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "Scripts" / "run_tests.sh"


class RunnerFailClosedTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = RUNNER.read_text(encoding="utf-8")

    def test_generated_docs_update_is_guarded_by_clean_process_and_logs(self):
        gate = "if (( OVERALL_RC == 0 && VERIFY_RC == 0 )); then"
        update = 'python "$ROOT/Scripts/update_docs.py"'
        self.assertEqual(self.source.count(update), 1)
        gate_index = self.source.find(gate)
        update_index = self.source.find(update)
        else_index = self.source.find("\nelse\n", gate_index)
        self.assertGreaterEqual(gate_index, 0)
        self.assertGreater(update_index, gate_index)
        self.assertGreater(else_index, update_index)

    def test_log_verification_status_starts_fail_closed(self):
        initialized = self.source.find("VERIFY_RC=81")
        log_gate = self.source.find('if [[ -s "$WOOD_LOG"')
        self.assertGreaterEqual(initialized, 0)
        self.assertGreater(log_gate, initialized)

    def test_failure_branch_explicitly_keeps_generated_docs_unchanged(self):
        self.assertIn(
            "UE process or result verification failed; generated docs were not updated.",
            self.source,
        )

    def test_unreal_process_codes_participate_in_overall_status(self):
        for marker in (
            '(( WOOD_RC == 0 )) || OVERALL_RC="$WOOD_RC"',
            'if (( STICK_RC != 0 && OVERALL_RC == 0 ))',
            'if (( INV_CAPACITY_RC != 0 && OVERALL_RC == 0 ))',
            'if (( STONE_RC != 0 && OVERALL_RC == 0 ))',
            'if (( TRAP_RC != 0 && OVERALL_RC == 0 ))',
            'if (( MECHANICS_RC != 0 && OVERALL_RC == 0 ))',
        ):
            self.assertIn(marker, self.source)


if __name__ == "__main__":
    unittest.main(verbosity=2)
