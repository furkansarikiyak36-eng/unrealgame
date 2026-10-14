#!/usr/bin/env python3
"""Audit a mechanic-catalog CSV without silently treating missing evidence as completion.

By default the command reports gaps and exits 0 so it can be used during discovery.
Pass --strict to make any schema, identity, or evidence gap fail the process.
"""
from __future__ import annotations

import argparse
import csv
import json
import sys
from collections import Counter
from pathlib import Path
from typing import Any

REQUIRED_COLUMNS = (
    "Mechanic_ID",
    "Sistem",
    "Katalog_ID",
    "Teslim_Fazi",
    "Mekanik",
    "Durum",
    "Kod_Kaniti_ve_Cagri_Zinciri",
    "Otomatik_Test_ID_ve_Sonuc",
    "Editor_Test_Kaniti",
    "Bagimlilik_Bloker",
    "Acik_Notlar",
)
EVIDENCE_BY_LEVEL = {
    0: (),
    1: ("Kod_Kaniti_ve_Cagri_Zinciri",),
    2: ("Kod_Kaniti_ve_Cagri_Zinciri",),
    3: ("Kod_Kaniti_ve_Cagri_Zinciri",),
    4: ("Kod_Kaniti_ve_Cagri_Zinciri", "Otomatik_Test_ID_ve_Sonuc"),
    5: ("Kod_Kaniti_ve_Cagri_Zinciri", "Otomatik_Test_ID_ve_Sonuc", "Editor_Test_Kaniti"),
    6: ("Kod_Kaniti_ve_Cagri_Zinciri", "Otomatik_Test_ID_ve_Sonuc", "Editor_Test_Kaniti"),
}


def _text(value: Any) -> str:
    return "" if value is None else str(value).strip()


def audit_rows(rows: list[dict[str, str]], headers: list[str]) -> dict[str, Any]:
    result: dict[str, Any] = {
        "row_count": len(rows),
        "errors": [],
        "warnings": [],
        "status_counts": {},
        "system_counts": {},
        "level_counts": {},
        "duplicate_ids": [],
        "missing_ids": [],
        "unmapped_rows": [],
        "evidence_gaps": [],
        "rows": [],
    }
    missing_columns = [c for c in REQUIRED_COLUMNS if c not in headers]
    if missing_columns:
        result["errors"].append({"type": "missing_columns", "columns": missing_columns})
        return result

    ids: list[str] = []
    statuses: Counter[str] = Counter()
    systems: Counter[str] = Counter()
    levels: Counter[str] = Counter()

    for line_no, row in enumerate(rows, start=2):
        mechanic_id = _text(row.get("Mechanic_ID"))
        system = _text(row.get("Sistem"))
        catalog_id = _text(row.get("Katalog_ID"))
        delivery_phase = _text(row.get("Teslim_Fazi"))
        mechanic = _text(row.get("Mekanik"))
        status = _text(row.get("Durum"))
        item_errors: list[str] = []
        item_warnings: list[str] = []

        if not mechanic_id:
            result["missing_ids"].append(line_no)
            item_errors.append("Mechanic_ID boş")
        else:
            ids.append(mechanic_id)
        if not system:
            item_errors.append("Sistem boş")
        if not catalog_id:
            item_errors.append("Katalog_ID boş")
        if not delivery_phase:
            item_errors.append("Teslim_Fazi boş")
        if not mechanic:
            item_errors.append("Mekanik boş")
        if not status:
            item_errors.append("Durum boş")

        level = None
        if status.startswith("U") and len(status) >= 2 and status[1].isdigit():
            level = int(status[1])
            if level > 6:
                item_errors.append(f"Geçersiz kanıt seviyesi: {level}")
                level = None
        else:
            item_warnings.append("Durum U0–U6 formatında değil")
        if level == 0 or status.startswith("U0"):
            item_warnings.append("Denetim gerekli: mekanik tamamlandı kabul edilemez")

        # Every catalog row must explicitly record its blocker/dependency state
        # and a note; U1+ also requires a traceable source/model location.
        missing_evidence = []
        for column in ("Bagimlilik_Bloker", "Acik_Notlar"):
            if not _text(row.get(column)):
                missing_evidence.append(column)
        if level is not None:
            for column in EVIDENCE_BY_LEVEL[level]:
                if not _text(row.get(column)):
                    missing_evidence.append(column)
        if missing_evidence:
            result["evidence_gaps"].append({
                "line": line_no, "mechanic_id": mechanic_id,
                "level": level, "missing": missing_evidence,
            })
            item_errors.append("Kanıt seviyesi için zorunlu alanlar eksik")

        if item_errors:
            result["errors"].append({
                "line": line_no, "mechanic_id": mechanic_id, "issues": item_errors,
            })
        if item_warnings:
            result["warnings"].append({
                "line": line_no, "mechanic_id": mechanic_id, "issues": item_warnings,
            })
        if item_errors or not mechanic_id or not status or level is None or level == 0 or missing_evidence:
            result["unmapped_rows"].append({
                "line": line_no, "mechanic_id": mechanic_id, "status": status,
            })

        statuses[status or "(boş)"] += 1
        systems[system or "(boş)"] += 1
        levels[f"U{level}" if level is not None else "Unclassified"] += 1
        result["rows"].append({
            "line": line_no, "mechanic_id": mechanic_id, "system": system,
            "catalog_id": catalog_id, "delivery_phase": delivery_phase,
            "status": status, "level": level, "issues": item_errors,
            "warnings": item_warnings,
        })

    counts = Counter(ids)
    result["duplicate_ids"] = sorted(x for x, n in counts.items() if n > 1)
    if result["duplicate_ids"]:
        result["errors"].append({
            "type": "duplicate_ids", "ids": result["duplicate_ids"],
        })

    result["status_counts"] = dict(sorted(statuses.items()))
    result["system_counts"] = dict(sorted(systems.items()))
    result["level_counts"] = dict(sorted(levels.items()))
    result["summary"] = {
        "total": len(rows),
        "unique_ids": len(set(ids)),
        "rows_with_errors": sum(1 for row in result["rows"] if row["issues"]),
        "rows_with_warnings": sum(1 for row in result["rows"] if row["warnings"]),
        "unmapped_or_unproven": len(result["unmapped_rows"]),
        "evidence_gaps": len(result["evidence_gaps"]),
        "duplicate_id_count": len(result["duplicate_ids"]),
    }
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_path", type=Path, help="Path to the mechanic catalog CSV")
    parser.add_argument("--strict", action="store_true",
                        help="Exit non-zero when schema, identity, or evidence gaps exist")
    parser.add_argument("--json-out", type=Path, help="Write full audit report as JSON")
    parser.add_argument("--expected-count", type=int, default=None,
                        help="Optional expected catalog row count")
    args = parser.parse_args()

    try:
        with args.csv_path.open("r", encoding="utf-8-sig", newline="") as f:
            reader = csv.DictReader(f)
            headers = list(reader.fieldnames or [])
            rows = list(reader)
    except OSError as exc:
        print(f"ERROR: cannot read {args.csv_path}: {exc}", file=sys.stderr)
        return 2
    except UnicodeError as exc:
        print(f"ERROR: invalid text encoding: {exc}", file=sys.stderr)
        return 2

    report = audit_rows(rows, headers)
    if args.expected_count is not None and len(rows) != args.expected_count:
        report["errors"].append({
            "type": "row_count_mismatch",
            "expected": args.expected_count,
            "actual": len(rows),
        })
    summary = report.get("summary", {})
    print(f"Catalog rows: {len(rows)}")
    print(f"Unique IDs: {summary.get('unique_ids', 0)}")
    print(f"Unmapped/unproven rows: {summary.get('unmapped_or_unproven', 'schema-error')}")
    print(f"Evidence gaps: {summary.get('evidence_gaps', 'schema-error')}")
    print(f"Duplicate IDs: {summary.get('duplicate_id_count', 'schema-error')}")
    print(f"Schema/identity errors: {len(report['errors'])}")
    print("\nStatus counts:")
    for status, count in report.get("status_counts", {}).items():
        print(f"  {status}: {count}")

    if args.json_out:
        try:
            args.json_out.parent.mkdir(parents=True, exist_ok=True)
            args.json_out.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                                     encoding="utf-8")
        except OSError as exc:
            print(f"ERROR: cannot write report: {exc}", file=sys.stderr)
            return 2

    has_gaps = bool(report["errors"] or report.get("unmapped_rows"))
    if args.strict and has_gaps:
        print("FAIL: strict audit found catalog gaps.", file=sys.stderr)
        return 1
    print("PASS: audit report generated; discovery gaps remain visible." if has_gaps
          else "PASS: catalog schema and evidence checks are clean.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
