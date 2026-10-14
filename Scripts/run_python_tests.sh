#!/usr/bin/env bash
# Platform-independent checks for repository Python utilities and catalog audit artifacts.
set -Eeuo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PYTHON="${PYTHON:-python}"
echo "[1/6] Verifying the UE-log parser test suite..."
"$PYTHON" "$ROOT/Scripts/tests/test_verify_test_logs.py"
echo "[2/6] Verifying the mechanic-catalog auditor test suite..."
"$PYTHON" "$ROOT/Scripts/tests/test_audit_mechanic_catalog.py"
echo "[3/6] Checking Python syntax..."
"$PYTHON" -m py_compile \
  "$ROOT/Scripts/verify_test_logs.py" \
  "$ROOT/Scripts/audit_mechanic_catalog.py" \
  "$ROOT/Scripts/tests/test_verify_test_logs.py" \
  "$ROOT/Scripts/tests/test_audit_mechanic_catalog.py"
echo "[4/6] Auditing the 76-row LOG evidence map (discovery mode)..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" \
  "$ROOT/Docs/FAZ_00_LOG_KATALOG_ESLESTIRME.csv" \
  --expected-count 76
echo "[5/6] Auditing the 32-row inventory evidence map (discovery mode)..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" \
  "$ROOT/Docs/FAZ_00_ENVANTER_KATALOG_ESLESTIRME.csv" \
  --expected-count 32
echo "[6/6] Auditing the 63-row RAID evidence map (discovery mode)..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" \
  "$ROOT/Docs/FAZ_00_RAID_KATALOG_ESLESTIRME.csv" \
  --expected-count 63
echo "ALL PYTHON TOOL TESTS AND CATALOG CHECKS PASSED."
