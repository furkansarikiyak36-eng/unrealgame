#!/usr/bin/env bash
# Build MyProject2 and run all phase-1 and phase-2 world automation tests.
# Intended for Windows Git Bash / MSYS with Unreal Engine 5.8 installed.
#
#   bash Scripts/run_phase2_tests.sh
#   UE_ENGINE_ROOT="/d/UE/UE_5.8/Engine" bash Scripts/run_phase2_tests.sh

set -Eeuo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="${UE_ENGINE_ROOT:-/c/Program Files/Epic Games/UE_5.8/Engine}"
PROJECT="$ROOT/MyProject2.uproject"
PROJECT_WIN="$(cygpath -w "$PROJECT")"
STAMP="$(date +%Y-%m-%d_%H%M%S)"
RUNS="$ROOT/Saved/TestRuns"
LOGS="$ROOT/Saved/Logs"
BUILD_LOG="$RUNS/phase2_build_$STAMP.log"
ENGINE_LOG="$LOGS/Phase2Tests.log"
SAVED_LOG="$RUNS/phase2_tests_$STAMP.log"

fail() {
  echo "HATA: $*" >&2
  exit 1
}

for tool in cygpath timeout grep; do
  command -v "$tool" >/dev/null 2>&1 || fail "Gerekli araç bulunamadı: $tool (Windows Git Bash/MSYS kullanın)."
done

[[ -f "$PROJECT" ]] || fail "Proje dosyası bulunamadı: $PROJECT"
[[ -f "$UE/Build/BatchFiles/Build.bat" ]] || fail "Build.bat bulunamadı: $UE/Build/BatchFiles/Build.bat"
[[ -f "$UE/Binaries/Win64/UnrealEditor-Cmd.exe" ]] || fail "UnrealEditor-Cmd.exe bulunamadı: $UE/Binaries/Win64/UnrealEditor-Cmd.exe"

mkdir -p "$RUNS" "$LOGS"

if command -v tasklist >/dev/null 2>&1 && tasklist 2>/dev/null | grep -qi "UnrealEditor.exe"; then
  fail "Unreal Editor açık. Önce Editor'ü kapatın."
fi

rm -f "$ENGINE_LOG"

echo "UE 5.8 build başlıyor; log: $BUILD_LOG"
if "$UE/Build/BatchFiles/Build.bat" MyProject2Editor Win64 Development \
    -Project="$PROJECT_WIN" -WaitMutex >"$BUILD_LOG" 2>&1; then
  grep -Ei "error|result" "$BUILD_LOG" || true
else
  rc=$?
  echo "Build başarısız (exit=$rc). Son 100 log satırı:" >&2
  tail -n 100 "$BUILD_LOG" >&2 || true
  exit "$rc"
fi

echo "Faz 2 Automation testleri başlıyor..."
if MSYS_NO_PATHCONV=1 timeout 400 "$UE/Binaries/Win64/UnrealEditor-Cmd.exe" "$PROJECT_WIN" \
    -ExecCmds="Automation RunTests MyProject2.World; Quit" \
    -unattended -nullrhi -nosplash -nop4 -log=Phase2Tests.log >/dev/null 2>&1; then
  :
else
  rc=$?
  echo "Faz 2 Automation süreci başarısız veya zaman aşımına uğradı (exit=$rc)." >&2
  tail -n 100 "$ENGINE_LOG" >&2 2>/dev/null || true
  exit "$rc"
fi

[[ -s "$ENGINE_LOG" ]] || fail "Faz 2 test sonucu log'u oluşturulmadı: $ENGINE_LOG"
cp "$ENGINE_LOG" "$SAVED_LOG"

EXPECTED_TESTS=(
  "MyProject2.World.RuntimeProfile.DefaultProfilesValidate"
  "MyProject2.World.RuntimeProfile.ConfigureAndLock"
  "MyProject2.World.Contracts.ValidateRequestsAndBuffers"
  "MyProject2.World.Classification.RegionTagPolicy"
  "MyProject2.World.Classification.ManualOverrideGrid"
  "MyProject2.World.Scan.DemandDrivenQueries"
)
for test_name in "${EXPECTED_TESTS[@]}"; do
  grep -Fq "$test_name" "$SAVED_LOG" || {
    echo "Beklenen otomasyon testi log'da bulunamadı: $test_name" >&2
    tail -n 100 "$SAVED_LOG" >&2 || true
    exit 1
  }
done

SUCCESS_COUNT="$(grep -a 'Test Completed' "$SAVED_LOG" | grep -c 'Success' || true)"
FAILURE_COUNT="$(grep -ac 'Result={Fail' "$SAVED_LOG" || true)"
if [[ "$SUCCESS_COUNT" -lt "${#EXPECTED_TESTS[@]}" || "$FAILURE_COUNT" -ne 0 ]]; then
  echo "Faz 2 testleri geçmedi/eksik: başarı=$SUCCESS_COUNT, failure kaydı=$FAILURE_COUNT, beklenen test=${#EXPECTED_TESTS[@]}" >&2
  tail -n 100 "$SAVED_LOG" >&2 || true
  exit 1
fi

echo "GEÇTİ — Faz 2 otomasyon testleri: $SUCCESS_COUNT başarı, $FAILURE_COUNT failure kaydı"
echo "Kayıt: $SAVED_LOG"
