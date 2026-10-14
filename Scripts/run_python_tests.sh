#!/usr/bin/env bash
# Platform-independent checks for repository Python utilities and catalog audit artifacts.
set -Eeuo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PYTHON="${PYTHON:-python}"

echo "[1/21] Verifying the UE-log parser test suite..."
"$PYTHON" "$ROOT/Scripts/tests/test_verify_test_logs.py"

echo "[2/21] Verifying the mechanic-catalog auditor test suite..."
"$PYTHON" "$ROOT/Scripts/tests/test_audit_mechanic_catalog.py"

echo "[3/21] Verifying the excavation spike matrix validator..."
"$PYTHON" "$ROOT/Scripts/tests/test_validate_excavation_spike.py"

echo "[4/21] Testing canonical catalog cross-mapping validator..."
"$PYTHON" "$ROOT/Scripts/tests/test_validate_catalog_mapping.py"

echo "[5/21] Testing fail-closed documentation update gate..."
"$PYTHON" "$ROOT/Scripts/tests/test_runner_fail_closed.py"

echo "[6/21] Verifying the inventory drop bridge contract..."
"$PYTHON" "$ROOT/Scripts/tests/test_inventory_drop_bridge_contract.py"

echo "[7/21] Verifying the world-core persistence contract..."
"$PYTHON" "$ROOT/Scripts/tests/test_world_persistence_contract.py"

echo "[8/21] Checking Python and shell syntax..."
bash -n "$ROOT/Scripts/run_tests.sh"
bash -n "$ROOT/Scripts/run_python_tests.sh"
"$PYTHON" -m py_compile \
  "$ROOT/Scripts/verify_test_logs.py" \
  "$ROOT/Scripts/audit_mechanic_catalog.py" \
  "$ROOT/Scripts/validate_catalog_mapping.py" \
  "$ROOT/Scripts/validate_excavation_spike.py" \
  "$ROOT/Scripts/tests/test_verify_test_logs.py" \
  "$ROOT/Scripts/tests/test_runner_fail_closed.py" \
  "$ROOT/Scripts/tests/test_audit_mechanic_catalog.py" \
  "$ROOT/Scripts/tests/test_validate_catalog_mapping.py" \
  "$ROOT/Scripts/tests/test_validate_excavation_spike.py" \
  "$ROOT/Scripts/tests/test_inventory_drop_bridge_contract.py" \
  "$ROOT/Scripts/tests/test_world_persistence_contract.py"

echo "[9/21] Auditing the 76-row LOG map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_LOG_KATALOG_ESLESTIRME.csv" --expected-count 76

echo "[10/21] Auditing the 32-row INVENTORY map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_ENVANTER_KATALOG_ESLESTIRME.csv" --expected-count 32

echo "[11/21] Auditing the 63-row RAID map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_RAID_KATALOG_ESLESTIRME.csv" --expected-count 63

echo "[12/21] Auditing the 35-row WORLD map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_WORLD_KATALOG_ESLESTIRME.csv" --expected-count 35

echo "[13/21] Auditing the 38-row MATERIAL map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_MATERIAL_KATALOG_ESLESTIRME.csv" --expected-count 38

echo "[14/21] Auditing the 57-row FARMING map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_FARMING_KATALOG_ESLESTIRME.csv" --expected-count 57

echo "[15/21] Auditing the 12-row CORE map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_CORE_KATALOG_ESLESTIRME.csv" --expected-count 12

echo "[16/21] Auditing the 96-row STONE map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_STONE_KATALOG_ESLESTIRME.csv" --expected-count 96

echo "[17/21] Auditing the 76-row STICK map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_STICK_KATALOG_ESLESTIRME.csv" --expected-count 76

echo "[18/21] Validating Phase 1 excavation benchmark matrix..."
"$PYTHON" "$ROOT/Scripts/validate_excavation_spike.py" "$ROOT/Docs/FAZ_01_KAZI_SPIKE_SABLONU.csv"

echo "[19/21] Reconciling the canonical catalog against its nine detailed mappings..."
"$PYTHON" "$ROOT/Scripts/validate_catalog_mapping.py" --master "$ROOT/Docs/MEKANIK_KATALOG_TAKIP_CSV.csv" --mapping-dir "$ROOT/Docs"

echo "[20/21] Auditing the canonical 485-row master catalog evidence state..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/MEKANIK_KATALOG_TAKIP_CSV.csv" --expected-count 485

echo "[21/21] Auditing the Phase 0 findings matrix..."
"$PYTHON" "$ROOT/Scripts/audit_findings_matrix.py" "$ROOT/Docs/FAZ_00_BULGU_MATRISI.csv"

echo "ALL PYTHON TOOL TESTS, INVENTORY DROP CONTRACT, WORLD CORE PERSISTENCE CONTRACT, MASTER CATALOG CROSS-MAPPING, NINE ROW MAP AUDITS, EXCAVATION SPIKE AND FINDINGS MATRIX CHECKS PASSED."
