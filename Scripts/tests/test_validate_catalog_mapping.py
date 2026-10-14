#!/usr/bin/env python3
"""Tests for canonical-to-detailed catalog mapping integrity."""
import csv
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from validate_catalog_mapping import MAPPING_FILES, audit_catalogs  # noqa: E402

COLUMNS = ["Mechanic_ID", "Sistem", "Mekanik", "Durum"]
SYSTEMS = tuple(MAPPING_FILES)


def write_csv(path: Path, headers: list[str], rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=headers)
        writer.writeheader()
        writer.writerows(rows)


class CatalogMappingTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.master_path = self.root / "MEKANIK_KATALOG_TAKIP_CSV.csv"
        self.mapping_dir = self.root / "Docs"
        self.expected = {system: 1 for system in SYSTEMS}
        self.master_rows = [
            {"Mechanic_ID": f"{system}-001", "Sistem": system, "Mekanik": f"{system} feature", "Durum": "U0"}
            for system in SYSTEMS
        ]
        self.mapping_rows = {system: [dict(row)] for system, row in zip(SYSTEMS, self.master_rows)}
        self.write_all()

    def tearDown(self):
        self.temp.cleanup()

    def write_all(self):
        write_csv(self.master_path, ["index"] + COLUMNS, [
            {"index": str(i), **row} for i, row in enumerate(self.master_rows)
        ])
        for system, filename in MAPPING_FILES.items():
            write_csv(self.mapping_dir / filename, COLUMNS, self.mapping_rows[system])

    def audit(self):
        return audit_catalogs(self.master_path, self.mapping_dir, self.expected)

    def test_valid_master_and_nine_mappings_pass(self):
        self.assertEqual(self.audit(), [])

    def test_duplicate_master_ids_fail(self):
        self.master_rows.append(dict(self.master_rows[0]))
        self.write_all()
        self.assertTrue(any("duplicate master" in e for e in self.audit()))

    def test_missing_master_system_row_fails_expected_count(self):
        self.master_rows = [row for row in self.master_rows if row["Sistem"] != "RAID"]
        self.write_all()
        self.assertTrue(any("master catalog expected" in e for e in self.audit()))
        self.assertTrue(any("master RAID rows expected" in e for e in self.audit()))

    def test_missing_id_from_mapping_fails(self):
        self.mapping_rows["STONE"] = []
        self.write_all()
        errors = self.audit()
        self.assertTrue(any("STONE mapping expected 1 rows, got 0" in e for e in errors))
        self.assertTrue(any("STONE mapping missing master IDs" in e for e in errors))

    def test_unregistered_mapping_id_fails(self):
        self.mapping_rows["RAID"] = [{
            "Mechanic_ID": "RAID-999", "Sistem": "RAID", "Mekanik": "Invented", "Durum": "U0"
        }]
        self.write_all()
        self.assertTrue(any("RAID mapping has unknown IDs" in e for e in self.audit()))

    def test_system_mismatch_fails(self):
        self.mapping_rows["CORE"][0]["Sistem"] = "WORLD"
        self.write_all()
        self.assertTrue(any("says system" in e for e in self.audit()))

    def test_duplicate_ids_across_mappings_fail(self):
        self.mapping_rows["WORLD"] = [dict(self.mapping_rows["LOG"][0])]
        self.write_all()
        self.assertTrue(any("IDs repeated across system mappings" in e for e in self.audit()))

    def test_missing_required_column_fails(self):
        write_csv(self.master_path, ["Mechanic_ID", "Sistem"], self.master_rows)
        self.assertTrue(any("missing required columns" in e for e in self.audit()))


if __name__ == "__main__":
    unittest.main(verbosity=2)
