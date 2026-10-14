#!/bin/bash
# Reproducible headless checks for MyProject2 (run in Git Bash on the UE 5.8 host).
# The script validates process exit codes AND the result lines in fresh test logs.
# Unreal Editor must be closed so the tested DLL is not stale.
#
#   bash Scripts/run_tests.sh            # build + Wood + crafting + inventory capacity + stone + trap + MechanicsDemo
#   bash Scripts/run_tests.sh --no-build # skip only the build step
set -Eeuo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="${UE_ENGINE_ROOT:-/c/Program Files/Epic Games/UE_5.8/Engine}"
PROJECT="$ROOT/MyProject2.uproject"
PROJECT_WIN="$(cygpath -w "$PROJECT")"
STAMP="$(date +%Y-%m-%d_%H%M%S)"
RUNS="$ROOT/Saved/TestRuns"
LOGS="$ROOT/Saved/Logs"
BUILD_LOG="$RUNS/build_$STAMP.log"

if [[ $# -gt 1 || ( $# -eq 1 && "$1" != "--no-build" ) ]]; then
  echo "Usage: $0 [--no-build]" >&2
  exit 2
fi
SKIP_BUILD=0
if [[ ${1:-} == "--no-build" ]]; then SKIP_BUILD=1; fi

mkdir -p "$RUNS" "$LOGS"

echo "[1/10] Running platform-independent Python unit tests and catalog audits..."
bash "$ROOT/Scripts/run_python_tests.sh"

if [[ ! -f "$PROJECT" ]]; then
  echo "Project file not found: $PROJECT" >&2
  exit 3
fi
if [[ ! -f "$UE/Binaries/Win64/UnrealEditor-Cmd.exe" ]]; then
  echo "UnrealEditor-Cmd.exe not found under UE_ENGINE_ROOT='$UE'." >&2
  echo "Set UE_ENGINE_ROOT to the UE 5.8 Engine directory." >&2
  exit 4
fi
if tasklist 2>/dev/null | grep -qi "UnrealEditor.exe"; then
  echo "Unreal Editor is open. Close it before building/testing." >&2
  exit 5
fi

if (( SKIP_BUILD == 0 )); then
  echo "[2/10] Building MyProject2Editor..."
  rm -f "$ROOT"/Binaries/Win64/UnrealEditor-MyProject2.patch_*
  BUILD_CMD=("$UE/Build/BatchFiles/Build.bat" MyProject2Editor Win64 Development "-Project=$PROJECT_WIN" -WaitMutex)
  if "${BUILD_CMD[@]}" >"$BUILD_LOG" 2>&1; then
    grep -aiE "error|result|succeeded|failed" "$BUILD_LOG" || true
  else
    BUILD_RC=$?
    grep -aiE "error|result|succeeded|failed" "$BUILD_LOG" || tail -n 80 "$BUILD_LOG"
    echo "Build failed (exit $BUILD_RC). Full log: $BUILD_LOG" >&2
    exit "$BUILD_RC"
  fi
else
  echo "[2/10] Build skipped by --no-build."
fi

run_ue_test() {
  local label="$1"
  local timeout_seconds="$2"
  local source_log="$3"
  local destination_log="$4"
  shift 4
  local source_path="$LOGS/$source_log"
  local destination_path="$RUNS/$destination_log"
  local process_rc=0

  rm -f "$source_path" "$destination_path"
  echo "Running $label..."
  if MSYS_NO_PATHCONV=1 timeout "$timeout_seconds" "$UE/Binaries/Win64/UnrealEditor-Cmd.exe" "$PROJECT_WIN" "$@" > /dev/null 2>&1; then
    process_rc=0
  else
    process_rc=$?
  fi

  if [[ -s "$source_path" ]]; then
    cp "$source_path" "$destination_path"
  else
    echo "$label produced no expected log: $source_path" >&2
    return 80
  fi

  if (( process_rc != 0 )); then
    echo "$label process failed or timed out (exit $process_rc). Log: $destination_path" >&2
    return "$process_rc"
  fi
  return 0
}

echo "[3/10] Running Wood automation tests..."
if run_ue_test "Wood automation" 400 "WoodTests.log" "wood_$STAMP.log" \
  -ExecCmds="Automation RunTests MyProject2.Wood; Quit" -unattended -nullrhi -nosplash -nop4 -log=WoodTests.log; then
  WOOD_RC=0
else
  WOOD_RC=$?
fi

echo "[4/10] Running stick crafting transaction automation tests..."
if run_ue_test "Stick inventory automation" 300 "StickInventoryTests.log" "stick_inventory_$STAMP.log" \
  -ExecCmds="Automation RunTests MyProject2.Inventory.StickCraftTransaction; Quit" -unattended -nullrhi -nosplash -nop4 -log=StickInventoryTests.log; then
  STICK_RC=0
else
  STICK_RC=$?
fi

echo "[5/10] Running inventory capacity automation tests..."
if run_ue_test "Inventory capacity automation" 300 "InventoryCapacityTests.log" "inventory_capacity_$STAMP.log" \
  -ExecCmds="Automation RunTests MyProject2.Inventory.Core.Capacity; Quit" -unattended -nullrhi -nosplash -nop4 -log=InventoryCapacityTests.log; then
  INV_CAPACITY_RC=0
else
  INV_CAPACITY_RC=$?
fi

echo "[6/10] Running stone placement automation tests..."
if run_ue_test "Stone placement automation" 300 "StonePlacementTests.log" "stone_placement_$STAMP.log" \
  -ExecCmds="Automation RunTests MyProject2.StonePlacement; Quit" -unattended -nullrhi -nosplash -nop4 -log=StonePlacementTests.log; then
  STONE_RC=0
else
  STONE_RC=$?
fi

echo "[7/10] Running stone trap damage automation tests..."
if run_ue_test "Stone trap automation" 300 "StoneTrapTests.log" "stone_trap_$STAMP.log" \
  -ExecCmds="Automation RunTests MyProject2.StoneTrap; Quit" -unattended -nullrhi -nosplash -nop4 -log=StoneTrapTests.log; then
  TRAP_RC=0
else
  TRAP_RC=$?
fi

echo "[8/10] Running MechanicsDemo checks..."
if run_ue_test "MechanicsDemo" 600 "MyProject2.log" "mekanik_$STAMP.log" \
  /Game/Maps/Demo_WoodChain -game -unattended -nullrhi -nosplash -nop4 -log -MechanicsDemoQuit; then
  MECHANICS_RC=0
else
  MECHANICS_RC=$?
fi

echo "[9/10] Verifying all fresh logs and updating generated docs..."
WOOD_LOG="$RUNS/wood_$STAMP.log"
STICK_LOG="$RUNS/stick_inventory_$STAMP.log"
STONE_LOG="$RUNS/stone_placement_$STAMP.log"
TRAP_LOG="$RUNS/stone_trap_$STAMP.log"
INV_CAPACITY_LOG="$RUNS/inventory_capacity_$STAMP.log"
MECHANICS_LOG="$RUNS/mekanik_$STAMP.log"
OVERALL_RC=0
(( WOOD_RC == 0 )) || OVERALL_RC="$WOOD_RC"
if (( STICK_RC != 0 && OVERALL_RC == 0 )); then OVERALL_RC="$STICK_RC"; fi
if (( STONE_RC != 0 && OVERALL_RC == 0 )); then OVERALL_RC="$STONE_RC"; fi
if (( INV_CAPACITY_RC != 0 && OVERALL_RC == 0 )); then OVERALL_RC="$INV_CAPACITY_RC"; fi
if (( TRAP_RC != 0 && OVERALL_RC == 0 )); then OVERALL_RC="$TRAP_RC"; fi
if (( MECHANICS_RC != 0 && OVERALL_RC == 0 )); then OVERALL_RC="$MECHANICS_RC"; fi

if [[ -s "$WOOD_LOG" && -s "$STICK_LOG" && -s "$INV_CAPACITY_LOG" && -s "$STONE_LOG" && -s "$TRAP_LOG" && -s "$MECHANICS_LOG" ]]; then
  if python "$ROOT/Scripts/verify_test_logs.py" \
    --mechanics "$MECHANICS_LOG" --wood "$WOOD_LOG" \
    --stick-inventory "$STICK_LOG" --inventory-capacity "$INV_CAPACITY_LOG" \
    --stone-placement "$STONE_LOG" \
    --stone-trap "$TRAP_LOG"; then
    VERIFY_RC=0
  else
    VERIFY_RC=$?
    if (( OVERALL_RC == 0 )); then OVERALL_RC="$VERIFY_RC"; fi
  fi
  python "$ROOT/Scripts/update_docs.py"
else
  echo "Fresh test logs are incomplete; refusing to generate a passing baseline." >&2
  if (( OVERALL_RC == 0 )); then OVERALL_RC=81; fi
fi

echo "[10/10] Pruning old logs..."
for pattern in "mekanik_*.log" "wood_*.log" "stick_inventory_*.log" "inventory_capacity_*.log" "stone_placement_*.log" "stone_trap_*.log" "build_*.log"; do
  ls -1t "$RUNS"/$pattern 2>/dev/null | tail -n +16 | xargs -r rm -f || true
done

if (( OVERALL_RC != 0 )); then
  echo "TEST RUN FAILED (exit $OVERALL_RC). Inspect logs in $RUNS." >&2
  exit "$OVERALL_RC"
fi

echo "ALL HEADLESS TESTS PASSED."
