#!/usr/bin/env python3
"""Unit tests for the Phase 0 findings matrix auditor."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from audit_findings_matrix import REQUIRED_COLUMNS, audit_rows  # noqa: E402


def row(finding_id="P0-001", **overrides):
    values = {
        "Finding_ID": finding_id,
        "System": "Inventory",
        "Severity": "High",
        "Repository_Evidence": "Source/MyProject2/InventoryCore/InventoryComponent.cpp",
        "Observed_Fact": "A concrete source behavior was inspected.",
        "What_Is_Not_Proven": "The end-to-end player path has not been proven.",
        "Required_Action": "Trace the caller and add a targeted test.",
        "Acceptance_Evidence": "A named test passes and its log is retained.",
        "Status": "Open",
    }
    values.update(overrides)
    return [values[column] for column in REQUIRED_COLUMNS]


class FindingsMatrixAuditTests(unittest.TestCase):
    def test_valid_contiguous_matrix_passes(self):
        errors = audit_rows([row("P0-001"), row("P0-002")], list(REQUIRED_COLUMNS))
        self.assertEqual(errors, [])

    def test_missing_finding_id_is_reported_as_sequence_gap(self):
        errors = audit_rows([row("P0-001"), row("P0-003")], list(REQUIRED_COLUMNS))
        self.assertTrue(any("P0-002" in error for error in errors))

    def test_duplicate_finding_ids_are_rejected(self):
        errors = audit_rows([row("P0-001"), row("P0-001")], list(REQUIRED_COLUMNS))
        self.assertTrue(any("duplicate Finding_ID" in error for error in errors))

    def test_status_cannot_swallow_another_finding(self):
        errors = audit_rows(
            [row("P0-001", Status=r"Open\nP0-002")],
            list(REQUIRED_COLUMNS),
        )
        self.assertTrue(any("Status must be one line" in error for error in errors))

    def test_severity_whitespace_is_rejected(self):
        errors = audit_rows([row("P0-001", Severity=" Critical")], list(REQUIRED_COLUMNS))
        self.assertTrue(any("surrounding whitespace" in error for error in errors))

    def test_empty_required_field_is_rejected(self):
        errors = audit_rows([row("P0-001", Acceptance_Evidence="")], list(REQUIRED_COLUMNS))
        self.assertTrue(any("Acceptance_Evidence is empty" in error for error in errors))

    def test_incorrect_row_width_is_rejected(self):
        values = row("P0-001")[:-1]
        errors = audit_rows([values], list(REQUIRED_COLUMNS))
        self.assertTrue(any("expected 9 fields, got 8" in error for error in errors))

    def test_missing_required_column_is_rejected(self):
        headers = [column for column in REQUIRED_COLUMNS if column != "Status"]
        errors = audit_rows([row("P0-001")[:-1]], headers)
        self.assertTrue(any("missing required columns: Status" in error for error in errors))

    def test_invalid_severity_is_rejected(self):
        errors = audit_rows([row("P0-001", Severity="Urgent")], list(REQUIRED_COLUMNS))
        self.assertTrue(any("invalid Severity" in error for error in errors))


if __name__ == "__main__":
    unittest.main(verbosity=2)
