#!/usr/bin/env python3
"""Unit tests for audit_mechanic_catalog.py; no Unreal Engine runtime required."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from audit_mechanic_catalog import REQUIRED_COLUMNS, audit_rows  # noqa: E402


def row(**overrides):
    value = {column: "" for column in REQUIRED_COLUMNS}
    value.update({
        "Mechanic_ID": "LOG-001",
        "Sistem": "LOG",
        "Mekanik": "Ağacı kesme",
        "Durum": "U5 — görsel olarak doğrulanmış",
        "Kod_Kaniti_ve_Cagri_Zinciri": "ForestCharacter -> TreeActor::Chop",
        "Otomatik_Test_ID_ve_Sonuc": "Mechanics.Tree.Chop PASS",
        "Editor_Test_Kaniti": "Harita: Test_Tree; editör kontrolü kaydedildi",
        "Bagimlilik_Bloker": "Yok",
        "Acik_Notlar": "Kanıt bağlantıları faz kaydında",
    })
    value.update(overrides)
    return value


class CatalogAuditTests(unittest.TestCase):
    def test_complete_u5_row_has_no_evidence_gap(self):
        report = audit_rows([row()], list(REQUIRED_COLUMNS))
        self.assertEqual(report["summary"]["evidence_gaps"], 0)
        self.assertEqual(report["summary"]["unmapped_or_unproven"], 0)
        self.assertEqual(report["errors"], [])

    def test_u0_is_visible_as_unproven_not_passed(self):
        report = audit_rows([row(
            Durum="U0 — denetim gerekli",
            Kod_Kaniti_ve_Cagri_Zinciri="",
            Otomatik_Test_ID_ve_Sonuc="",
            Editor_Test_Kaniti="",
        )], list(REQUIRED_COLUMNS))
        self.assertEqual(report["summary"]["unmapped_or_unproven"], 1)
        self.assertTrue(any("Denetim gerekli" in issue
                            for item in report["warnings"] for issue in item["issues"]))

    def test_u4_requires_code_and_automation_evidence(self):
        report = audit_rows([row(
            Durum="U4 — otomatik testli",
            Kod_Kaniti_ve_Cagri_Zinciri="",
            Otomatik_Test_ID_ve_Sonuc="",
        )], list(REQUIRED_COLUMNS))
        self.assertEqual(report["summary"]["evidence_gaps"], 1)
        self.assertTrue(report["errors"])

    def test_duplicate_mechanic_ids_are_reported(self):
        report = audit_rows([row(), row(Sistem="STONE", Mekanik="Taş toplama")],
                            list(REQUIRED_COLUMNS))
        self.assertEqual(report["duplicate_ids"], ["LOG-001"])
        self.assertTrue(any(error.get("type") == "duplicate_ids"
                            for error in report["errors"]))

    def test_missing_required_column_fails_schema(self):
        headers = [column for column in REQUIRED_COLUMNS if column != "Mekanik"]
        report = audit_rows([row()], headers)
        self.assertEqual(report["errors"][0]["type"], "missing_columns")
        self.assertIn("Mekanik", report["errors"][0]["columns"])

    def test_unknown_status_is_warning_and_unclassified(self):
        report = audit_rows([row(Durum="implemented-ish")], list(REQUIRED_COLUMNS))
        self.assertEqual(report["level_counts"]["Unclassified"], 1)
        self.assertEqual(report["summary"]["unmapped_or_unproven"], 1)

    def test_duplicate_ids_in_different_systems_still_fail(self):
        report = audit_rows([
            row(Mechanic_ID="CORE-001", Sistem="CORE"),
            row(Mechanic_ID="CORE-001", Sistem="RAID"),
        ], list(REQUIRED_COLUMNS))
        self.assertEqual(report["duplicate_ids"], ["CORE-001"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
