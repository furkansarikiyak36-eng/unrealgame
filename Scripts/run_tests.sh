#!/bin/bash
# Reproducible headless checks for MyProject2 (run in Git Bash on the UE 5.8 host).
# The script validates process exit codes AND the result lines in both test logs.
# Unreal Editor must be closed so the tested DLL is not stale.
#
#   bash Scripts/run_tests.sh            # build + Wood automation + MechanicsDemo
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

# Fast tests run first so failures are detected before spending time on an engine build.
echo "[1/6] Running platform-independent Python unit tests..."
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
  echo "[2/6] Building MyProject2Editor..."
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
  echo "[2/6] Build skipped by --no-build."
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

echo "[3/6] Running Wood automation tests..."
if run_ue_test "Wood automation" 400 "WoodTests.log" "wood_$STAMP.log" \
  -ExecCmds="Automation RunTests MyProject2.Wood; Quit" -unattended -nullrhi -nosplash -nop4 -log=WoodTests.log; then
  WOOD_RC=0
else
  WOOD_RC=$?
fi

echo "[4/6] Running MechanicsDemo checks..."
if run_ue_test "MechanicsDemo" 600 "MyProject2.log" "mekanik_$STAMP.log" \
  /Game/Maps/Demo_WoodChain -game -unattended -nullrhi -nosplash -nop4 -log -MechanicsDemoQuit; then
  MECHANICS_RC=0
else
  MECHANICS_RC=$?
fi

echo "[5/6] Verifying fresh logs and updating generated docs..."
WOOD_LOG="$RUNS/wood_$STAMP.log"
MECHANICS_LOG="$RUNS/mekanik_$STAMP.log"
OVERALL_RC=0
(( WOOD_RC == 0 )) || OVERALL_RC="$WOOD_RC"
if (( MECHANICS_RC != 0 && OVERALL_RC == 0 )); then OVERALL_RC="$MECHANICS_RC"; fi

if [[ -s "$WOOD_LOG" && -s "$MECHANICS_LOG" ]]; then
  if python "$ROOT/Scripts/verify_test_logs.py" --mechanics "$MECHANICS_LOG" --wood "$WOOD_LOG"; then
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

echo "[6/6] Pruning old logs..."
# Keep the latest 15 run logs of each kind and the latest 15 build logs.
for pattern in "mekanik_*.log" "wood_*.log" "build_*.log"; do
  ls -1t "$RUNS"/$pattern 2>/dev/null | tail -n +16 | xargs -r rm -f || true
done

if (( OVERALL_RC != 0 )); then
  echo "TEST RUN FAILED (exit $OVERALL_RC). Inspect logs in $RUNS." >&2
  exit "$OVERALL_RC"
fi

echo "ALL HEADLESS TESTS PASSED."
