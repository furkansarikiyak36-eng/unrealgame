#!/usr/bin/env bash
# Platform-independent unit tests for repository Python utilities.
set -Eeuo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PYTHON="${PYTHON:-python}"
echo "[1/3] Verifying the UE-log parser test suite..."
"$PYTHON" "$ROOT/Scripts/tests/test_verify_test_logs.py"
echo "[2/3] Verifying the mechanic-catalog auditor test suite..."
"$PYTHON" "$ROOT/Scripts/tests/test_audit_mechanic_catalog.py"
echo "[3/3] Checking Python syntax..."
"$PYTHON" -m py_compile \
  "$ROOT/Scripts/verify_test_logs.py" \
  "$ROOT/Scripts/audit_mechanic_catalog.py" \
  "$ROOT/Scripts/tests/test_verify_test_logs.py" \
  "$ROOT/Scripts/tests/test_audit_mechanic_catalog.py"
echo "ALL PYTHON UNIT TESTS PASSED."
