#!/usr/bin/env python3
"""Cross-check the canonical mechanic catalog against all nine repo mappings."""
from __future__ import annotations

import argparse
import csv
import sys
from collections import Counter
from pathlib import Path

EXPECTED_COUNTS = {
    "LOG": 76,
    "STONE": 96,
    "STICK": 76,
    "INVENTORY": 32,
    "RAID": 63,
    "FARMING": 57,
    "WORLD": 35,
    "MATERIAL": 38,
    "CORE": 12,
}
MAPPING_FILES = {
    "LOG": "FAZ_00_LOG_KATALOG_ESLESTIRME.csv",
    "STONE": "FAZ_00_STONE_KATALOG_ESLESTIRME.csv",
    "STICK": "FAZ_00_STICK_KATALOG_ESLESTIRME.csv",
    "INVENTORY": "FAZ_00_ENVANTER_KATALOG_ESLESTIRME.csv",
    "RAID": "FAZ_00_RAID_KATALOG_ESLESTIRME.csv",
    "FARMING": "FAZ_00_FARMING_KATALOG_ESLESTIRME.csv",
    "WORLD": "FAZ_00_WORLD_KATALOG_ESLESTIRME.csv",
    "MATERIAL": "FAZ_00_MATERIAL_KATALOG_ESLESTIRME.csv",
    "CORE": "FAZ_00_CORE_KATALOG_ESLESTIRME.csv",
}
REQUIRED_MASTER_COLUMNS = {"Mechanic_ID", "Sistem", "Mekanik", "Durum"}
REQUIRED_MAPPING_COLUMNS = {"Mechanic_ID", "Sistem", "Mekanik", "Durum"}


def read_csv(path: Path, required: set[str]) -> tuple[list[dict[str, str]], list[str], list[str]]:
    try:
        with path.open("r", encoding="utf-8-sig", newline="") as handle:
            reader = csv.DictReader(handle)
            headers = list(reader.fieldnames or [])
            rows = list(reader)
    except (OSError, UnicodeError, csv.Error) as exc:
        return [], [], [f"{path}: cannot read CSV: {exc}"]
    missing = sorted(required - set(headers))
    errors = [f"{path}: missing required columns: {', '.join(missing)}"] if missing else []
    return rows, headers, errors


def audit_catalogs(
    master_path: Path,
    mapping_dir: Path,
    expected_counts: dict[str, int] | None = None,
) -> list[str]:
    expected = expected_counts or EXPECTED_COUNTS
    errors: list[str] = []
    master_rows, master_headers, load_errors = read_csv(master_path, REQUIRED_MASTER_COLUMNS)
    errors.extend(load_errors)
    if load_errors:
        return errors

    if len(master_rows) != sum(expected.values()):
        errors.append(
            f"master catalog expected {sum(expected.values())} data rows, got {len(master_rows)}"
        )
    master_ids: list[str] = []
    ids_by_system: dict[str, set[str]] = {system: set() for system in expected}
    actual_system_counts: Counter[str] = Counter()
    for line, row in enumerate(master_rows, start=2):
        mechanic_id = (row.get("Mechanic_ID") or "").strip()
        system = (row.get("Sistem") or "").strip()
        if not mechanic_id:
            errors.append(f"{master_path}:{line}: Mechanic_ID is empty")
            continue
        if not system:
            errors.append(f"{master_path}:{line}: Sistem is empty for {mechanic_id}")
            continue
        master_ids.append(mechanic_id)
        actual_system_counts[system] += 1
        if system not in ids_by_system:
            errors.append(f"{master_path}:{line}: unexpected system {system!r} for {mechanic_id}")
        else:
            ids_by_system[system].add(mechanic_id)

    duplicated_master = sorted(key for key, count in Counter(master_ids).items() if count > 1)
    if duplicated_master:
        errors.append("duplicate master Mechanic_ID values: " + ", ".join(duplicated_master))
    for system, expected_count in expected.items():
        actual = actual_system_counts.get(system, 0)
        if actual != expected_count:
            errors.append(f"master {system} rows expected {expected_count}, got {actual}")

    all_mapping_ids: list[str] = []
    for system, filename in MAPPING_FILES.items():
        path = mapping_dir / filename
        mapping_rows, _, mapping_errors = read_csv(path, REQUIRED_MAPPING_COLUMNS)
        errors.extend(mapping_errors)
        if mapping_errors:
            continue
        if len(mapping_rows) != expected.get(system):
            errors.append(
                f"mapping {system} expected {expected.get(system)} rows, got {len(mapping_rows)} ({path})"
            )
        mapping_ids: list[str] = []
        for line, row in enumerate(mapping_rows, start=2):
            mechanic_id = (row.get("Mechanic_ID") or "").strip()
            mapped_system = (row.get("Sistem") or "").strip()
            if not mechanic_id:
                errors.append(f"{path}:{line}: Mechanic_ID is empty")
                continue
            mapping_ids.append(mechanic_id)
            all_mapping_ids.append(mechanic_id)
            if mapped_system != system:
                errors.append(
                    f"{path}:{line}: ID {mechanic_id} says system {mapped_system!r}, expected {system!r}"
                )
        duplicated_mapping = sorted(key for key, count in Counter(mapping_ids).items() if count > 1)
        if duplicated_mapping:
            errors.append(
                f"duplicate IDs in {system} mapping: " + ", ".join(duplicated_mapping)
            )
        expected_ids = ids_by_system.get(system, set())
        actual_ids = set(mapping_ids)
        missing = sorted(expected_ids - actual_ids)
        extra = sorted(actual_ids - expected_ids)
        if missing:
            errors.append(f"{system} mapping missing master IDs: " + ", ".join(missing))
        if extra:
            errors.append(f"{system} mapping has unknown IDs: " + ", ".join(extra))

    duplicated_across_mappings = sorted(
        key for key, count in Counter(all_mapping_ids).items() if count > 1
    )
    if duplicated_across_mappings:
        errors.append(
            "IDs repeated across system mappings: " + ", ".join(duplicated_across_mappings)
        )

    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--master", type=Path, required=True, help="Canonical 485-row CSV")
    parser.add_argument("--mapping-dir", type=Path, required=True, help="Directory with nine row-level mappings")
    args = parser.parse_args()

    errors = audit_catalogs(args.master, args.mapping_dir)
    if errors:
        print("CATALOG MAPPING INVALID:", file=sys.stderr)
        for error in errors:
            print(f" - {error}", file=sys.stderr)
        return 1

    print("Catalog identity mapping valid: 485 master rows, 485 unique IDs, nine mappings reconcile.")
    for system, count in EXPECTED_COUNTS.items():
        print(f"  {system}: {count}")
    print("PASS: this validates IDs and mapping coverage only; it does not imply mechanic behavior is tested.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
