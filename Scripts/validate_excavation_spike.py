#!/usr/bin/env python3
"""Validate the Phase 1 excavation benchmark matrix without fabricating measurements."""
from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path

REQUIRED_COLUMNS = (
    "Aday", "Çözünürlük", "Senaryo", "UE_Sürümü", "Commit_SHA", "Build_Config",
    "CPU_GPU_RAM", "Seed", "Volume_Chunk_Sayısı", "Vertex_Sayısı", "Triangle_Sayısı",
    "Kazı_GameThread_ms", "Mesh_Update_ms", "Collision_Update_ms",
    "Worst_Frame_Spike_ms", "Delta_or_Snapshot_Bytes", "Reload_ms", "Seam_Test",
    "Physics_Nav_Test", "Save_Load_Hash", "Kısmi_İşlem_Rollback", "Kanıt_Log_Link",
    "Açık_Risk", "Karar",
)
EXPECTED_PAIRS = {
    ("A_Sparse_3D_SDF", "25cm"),
    ("A_Sparse_3D_SDF", "50cm"),
    ("B_DynamicMesh_CSG", "25cm"),
    ("B_DynamicMesh_CSG", "50cm"),
}
MEASUREMENTS = (
    "UE_Sürümü", "Commit_SHA", "Build_Config", "CPU_GPU_RAM", "Seed",
    "Volume_Chunk_Sayısı", "Vertex_Sayısı", "Triangle_Sayısı",
    "Kazı_GameThread_ms", "Mesh_Update_ms", "Collision_Update_ms",
    "Worst_Frame_Spike_ms", "Delta_or_Snapshot_Bytes", "Reload_ms",
    "Seam_Test", "Physics_Nav_Test", "Save_Load_Hash",
    "Kısmi_İşlem_Rollback", "Kanıt_Log_Link", "Açık_Risk",
)


def audit_spike_rows(rows: list[dict[str, str]], headers: list[str]) -> list[str]:
    errors: list[str] = []
    missing = [column for column in REQUIRED_COLUMNS if column not in headers]
    if missing:
        return [f"missing columns: {', '.join(missing)}"]
    if len(rows) != 4:
        errors.append(f"expected exactly 4 benchmark rows (2 candidates x 2 resolutions), got {len(rows)}")

    seen: list[tuple[str, str]] = []
    for index, row in enumerate(rows, start=2):
        candidate = (row.get("Aday") or "").strip()
        resolution = (row.get("Çözünürlük") or "").strip()
        pair = (candidate, resolution)
        seen.append(pair)
        if pair not in EXPECTED_PAIRS:
            errors.append(f"line {index}: unexpected candidate/resolution pair {pair!r}")
        if not (row.get("Senaryo") or "").strip():
            errors.append(f"line {index}: scenario is empty")
        decision = (row.get("Karar") or "").strip().casefold()
        if decision in {"selected", "seçildi", "tamamlandı", "complete", "passed"}:
            blanks = [column for column in MEASUREMENTS if not (row.get(column) or "").strip()]
            if blanks:
                errors.append(
                    f"line {index}: completed measurement row has blank evidence fields: "
                    + ", ".join(blanks)
                )

    duplicate_pairs = sorted({pair for pair in seen if seen.count(pair) > 1})
    if duplicate_pairs:
        errors.append(f"duplicate candidate/resolution pairs: {duplicate_pairs}")
    missing_pairs = sorted(EXPECTED_PAIRS - set(seen))
    if missing_pairs:
        errors.append(f"missing candidate/resolution pairs: {missing_pairs}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_path", type=Path)
    args = parser.parse_args()
    try:
        with args.csv_path.open("r", encoding="utf-8-sig", newline="") as handle:
            reader = csv.DictReader(handle)
            headers = list(reader.fieldnames or [])
            rows = list(reader)
    except (OSError, UnicodeError) as exc:
        print(f"ERROR: cannot read spike matrix: {exc}", file=sys.stderr)
        return 2

    errors = audit_spike_rows(rows, headers)
    if errors:
        print("EXCAVATION SPIKE MATRIX INVALID:", file=sys.stderr)
        for error in errors:
            print(f" - {error}", file=sys.stderr)
        return 1

    print(f"Excavation spike matrix schema valid: {len(rows)} rows, 2 candidates x 2 resolutions.")
    print("Measurements are still pending until UE 5.8 evidence is recorded.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
