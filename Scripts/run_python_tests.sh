#!/usr/bin/env bash
# Platform-independent checks for repository Python utilities and catalog audit artifacts.
set -Eeuo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PYTHON="${PYTHON:-python}"

echo "[1/31] Verifying the UE-log parser test suite..."
"$PYTHON" "$ROOT/Scripts/tests/test_verify_test_logs.py"

echo "[2/31] Verifying the mechanic-catalog auditor test suite..."
"$PYTHON" "$ROOT/Scripts/tests/test_audit_mechanic_catalog.py"

echo "[3/31] Verifying the excavation spike matrix validator..."
"$PYTHON" "$ROOT/Scripts/tests/test_validate_excavation_spike.py"

echo "[4/31] Testing canonical catalog cross-mapping validator..."
"$PYTHON" "$ROOT/Scripts/tests/test_validate_catalog_mapping.py"

echo "[5/31] Testing fail-closed documentation update gate..."
"$PYTHON" "$ROOT/Scripts/tests/test_runner_fail_closed.py"

echo "[6/31] Verifying the inventory drop bridge contract..."
"$PYTHON" "$ROOT/Scripts/tests/test_inventory_drop_bridge_contract.py"

echo "[7/31] Verifying the world-core persistence contract..."
"$PYTHON" "$ROOT/Scripts/tests/test_world_persistence_contract.py"

echo "[8/31] Verifying the excavation terrain adapter contract..."
"$PYTHON" "$ROOT/Scripts/tests/test_excavation_terrain_adapter_contract.py"

echo "[9/31] Verifying the underground hazard contract..."
"$PYTHON" "$ROOT/Scripts/tests/test_underground_hazard_contract.py"

echo "[10/31] Verifying the carvable-log damage/recovery contract..."
"$PYTHON" "$ROOT/Scripts/tests/test_carvable_log_contract.py"

echo "[11/31] Verifying the inventory transfer transaction contract..."
"$PYTHON" "$ROOT/Scripts/tests/test_inventory_transfer_contract.py"

echo "[12/31] Verifying Phase 6 inventory, stick and persistence integration..."
"$PYTHON" "$ROOT/Scripts/tests/test_phase6_inventory_integration.py"

echo "[13/31] Verifying Phase 7 wood material/structure integration..."
"$PYTHON" "$ROOT/Scripts/tests/test_wood_material_integration_contract.py"

echo "[14/31] Verifying Phase 8 world-state integration and persistence..."
"$PYTHON" "$ROOT/Scripts/tests/test_phase8_world_state_integration_contract.py"

echo "[15/31] Verifying Phase 9 farming, animal feed, products and manure integration..."
"$PYTHON" "$ROOT/Scripts/tests/test_phase9_farming_integration_contract.py"

echo "[16/31] Verifying Phase 10 region, infrastructure and weighted route integration..."
"$PYTHON" "$ROOT/Scripts/tests/test_phase10_region_logistics_contract.py"

echo "[17/31] Verifying Phase 11 production queues, capacity, power, and save integration..."
"$PYTHON" "$ROOT/Scripts/tests/test_phase11_production_chain_contract.py"

echo "[18/31] Checking Python and shell syntax..."
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
  "$ROOT/Scripts/tests/test_world_persistence_contract.py" \
  "$ROOT/Scripts/tests/test_excavation_terrain_adapter_contract.py" \
  "$ROOT/Scripts/tests/test_underground_hazard_contract.py" \
  "$ROOT/Scripts/tests/test_carvable_log_contract.py" \
  "$ROOT/Scripts/tests/test_inventory_transfer_contract.py" \
  "$ROOT/Scripts/tests/test_phase6_inventory_integration.py" \
  "$ROOT/Scripts/tests/test_wood_material_integration_contract.py" \
  "$ROOT/Scripts/tests/test_phase8_world_state_integration_contract.py" \
  "$ROOT/Scripts/tests/test_phase9_farming_integration_contract.py" \
  "$ROOT/Scripts/tests/test_phase10_region_logistics_contract.py"
  "$ROOT/Scripts/tests/test_phase11_production_chain_contract.py"

echo "[19/31] Auditing the 76-row LOG map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_LOG_KATALOG_ESLESTIRME.csv" --expected-count 76

echo "[20/31] Auditing the 32-row INVENTORY map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_ENVANTER_KATALOG_ESLESTIRME.csv" --expected-count 32

echo "[21/31] Auditing the 63-row RAID map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_RAID_KATALOG_ESLESTIRME.csv" --expected-count 63

echo "[22/31] Auditing the 35-row WORLD map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_WORLD_KATALOG_ESLESTIRME.csv" --expected-count 35

echo "[23/31] Auditing the 38-row MATERIAL map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_MATERIAL_KATALOG_ESLESTIRME.csv" --expected-count 38

echo "[24/31] Auditing the 57-row FARMING map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_FARMING_KATALOG_ESLESTIRME.csv" --expected-count 57

echo "[25/31] Auditing the 12-row CORE map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_CORE_KATALOG_ESLESTIRME.csv" --expected-count 12

echo "[26/31] Auditing the 96-row STONE map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_STONE_KATALOG_ESLESTIRME.csv" --expected-count 96

echo "[27/31] Auditing the 76-row STICK map..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/FAZ_00_STICK_KATALOG_ESLESTIRME.csv" --expected-count 76

echo "[28/31] Validating Phase 1 excavation benchmark matrix..."
"$PYTHON" "$ROOT/Scripts/validate_excavation_spike.py" "$ROOT/Docs/FAZ_01_KAZI_SPIKE_SABLONU.csv"

echo "[29/31] Reconciling the canonical catalog against its nine detailed mappings..."
"$PYTHON" "$ROOT/Scripts/validate_catalog_mapping.py" --master "$ROOT/Docs/MEKANIK_KATALOG_TAKIP_CSV.csv" --mapping-dir "$ROOT/Docs"

echo "[30/31] Auditing the canonical 485-row master catalog evidence state..."
"$PYTHON" "$ROOT/Scripts/audit_mechanic_catalog.py" "$ROOT/Docs/MEKANIK_KATALOG_TAKIP_CSV.csv" --expected-count 485

echo "[31/31] Auditing the Phase 0 findings matrix..."
"$PYTHON" "$ROOT/Scripts/audit_findings_matrix.py" "$ROOT/Docs/FAZ_00_BULGU_MATRISI.csv"

echo "ALL PYTHON TOOL TESTS, INVENTORY DROP CONTRACT, INVENTORY TRANSFER TRANSACTION CONTRACT, PHASE 6 INVENTORY/STICK/SAVE CONTRACT, PHASE 7 WOOD MATERIAL/STRUCTURE CONTRACT, PHASE 8 WORLD-STATE INTEGRATION CONTRACT, PHASE 9 FARMING/ANIMAL/FEED/MANURE CONTRACT, PHASE 10 REGION/LOGISTICS CONTRACT, PHASE 11 PRODUCTION CHAIN CONTRACT, WORLD CORE PERSISTENCE CONTRACT, EXCAVATION TERRAIN ADAPTER CONTRACT, UNDERGROUND HAZARD CONTRACT, CARVABLE LOG REPAIR/SUPPORT CONTRACT, MASTER CATALOG CROSS-MAPPING, NINE ROW MAP AUDITS, EXCAVATION SPIKE AND FINDINGS MATRIX CHECKS PASSED."
