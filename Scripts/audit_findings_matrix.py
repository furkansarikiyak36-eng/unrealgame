#!/usr/bin/env python3
"""Validate the Phase 0 findings matrix for missing or corrupted blocker records."""
from __future__ import annotations

import argparse
import csv
import re
import sys
from pathlib import Path

REQUIRED_COLUMNS = (
    "Finding_ID",
    "System",
    "Severity",
    "Repository_Evidence",
    "Observed_Fact",
    "What_Is_Not_Proven",
    "Required_Action",
    "Acceptance_Evidence",
    "Status",
)
VALID_SEVERITIES = {"Critical", "High", "Medium", "Low"}
FINDING_ID_PATTERN = re.compile(r"P0-(\d{3})")


def audit_rows(rows: list[list[str]], headers: list[str]) -> list[str]:
    errors: list[str] = []
    missing_columns = [name for name in REQUIRED_COLUMNS if name not in headers]
    extra_columns = [name for name in headers if name not in REQUIRED_COLUMNS]
    if missing_columns:
        errors.append(f"missing required columns: {', '.join(missing_columns)}")
    if extra_columns:
        errors.append(f"unexpected columns: {', '.join(extra_columns)}")
    if len(headers) != len(set(headers)):
        errors.append("duplicate column names")
    if missing_columns or extra_columns or len(headers) != len(set(headers)):
        return errors
    if not rows:
        return ["findings matrix contains no records"]

    ids: list[str] = []
    for line_number, row in enumerate(rows, start=2):
        if len(row) != len(headers):
            errors.append(
                f"line {line_number}: expected {len(headers)} fields, got {len(row)}"
            )
            continue
        record = dict(zip(headers, row))
        for column in REQUIRED_COLUMNS:
            if not (record.get(column) or "").strip():
                errors.append(f"line {line_number} ({record.get('Finding_ID', '?')}): {column} is empty")

        finding_id = (record.get("Finding_ID") or "").strip()
        match = FINDING_ID_PATTERN.fullmatch(finding_id)
        if not match:
            errors.append(f"line {line_number}: invalid Finding_ID {finding_id!r}")
        else:
            number = int(match.group(1))
            if number < 1:
                errors.append(f"line {line_number}: Finding_ID number must be positive: {finding_id}")
            ids.append(finding_id)

        severity = record.get("Severity") or ""
        if severity != severity.strip():
            errors.append(f"line {line_number} ({finding_id}): Severity has surrounding whitespace")
        if severity.strip() not in VALID_SEVERITIES:
            errors.append(f"line {line_number} ({finding_id}): invalid Severity {severity!r}")

        status = record.get("Status") or ""
        if "\n" in status or "\r" in status or re.search(r"\\nP0-\d{3}", status):
            errors.append(f"line {line_number} ({finding_id}): Status must be one line and contain one finding only")

    duplicate_ids = sorted({item for item in ids if ids.count(item) > 1})
    if duplicate_ids:
        errors.append("duplicate Finding_ID values: " + ", ".join(duplicate_ids))

    numeric_ids = sorted({int(item[3:]) for item in ids})
    if numeric_ids:
        expected = set(range(1, max(numeric_ids) + 1))
        missing_numbers = sorted(expected - set(numeric_ids))
        if missing_numbers:
            missing = ", ".join(f"P0-{number:03d}" for number in missing_numbers)
            errors.append("Finding_ID sequence has gaps: " + missing)
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_path", type=Path, help="Path to Docs/FAZ_00_BULGU_MATRISI.csv")
    args = parser.parse_args()
    try:
        with args.csv_path.open("r", encoding="utf-8-sig", newline="") as handle:
            reader = csv.reader(handle)
            headers = next(reader, [])
            rows = [row for row in reader if any(cell.strip() for cell in row)]
    except (OSError, UnicodeError, csv.Error) as exc:
        print(f"ERROR: cannot read findings matrix: {exc}", file=sys.stderr)
        return 2

    errors = audit_rows(rows, headers)
    if errors:
        print("FINDINGS MATRIX INVALID:", file=sys.stderr)
        for error in errors:
            print(f" - {error}", file=sys.stderr)
        return 1

    print(f"Findings matrix valid: {len(rows)} records, {len({r[0].strip() for r in rows})} unique IDs.")
    print("PASS: finding IDs are contiguous; schema, severity, and status integrity checks are clean.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
