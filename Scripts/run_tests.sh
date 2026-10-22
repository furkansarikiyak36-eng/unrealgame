#!/usr/bin/env bash
# Build MyProject2, run the Wood automation suite and the guided mechanics demo,
# validate the recorded results, and refresh generated test/code-map documents.
#
# Run from Windows Git Bash / MSYS with Unreal Engine 5.8 installed:
#   bash Scripts/run_tests.sh
#   bash Scripts/run_tests.sh --no-build
#
# Override the engine root when installed elsewhere (MSYS path format):
#   UE_ENGINE_ROOT="/d/UE/UE_5.8/Engine" bash Scripts/run_tests.sh

set -Eeuo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="${UE_ENGINE_ROOT:-/c/Program Files/Epic Games/UE_5.8/Engine}"
PROJECT="$ROOT/MyProject2.uproject"
PROJECT_WIN="$(cygpath -w "$PROJECT")"
STAMP="$(date +%Y-%m-%d_%H%M%S)"
RUNS="$ROOT/Saved/TestRuns"
LOGS="$ROOT/Saved/Logs"
BUILD_LOG="$RUNS/build_$STAMP.log"
WOOD_RESULT_LOG="$LOGS/WoodTests.log"
MECHANICS_RESULT_LOG="$LOGS/MyProject2.log"
WOOD_SAVED_LOG="$RUNS/wood_$STAMP.log"
MECHANICS_SAVED_LOG="$RUNS/mekanik_$STAMP.log"

fail() {
  echo "HATA: $*" >&2
  exit 1
}

for tool in cygpath timeout python grep; do
  command -v "$tool" >/dev/null 2>&1 || fail "Gerekli araç bulunamadı: $tool (Windows Git Bash/MSYS kullanın)."
done

[[ -f "$PROJECT" ]] || fail "Proje dosyası bulunamadı: $PROJECT"
[[ -f "$UE/Build/BatchFiles/Build.bat" ]] || fail "Build.bat bulunamadı: $UE/Build/BatchFiles/Build.bat"
[[ -f "$UE/Binaries/Win64/UnrealEditor-Cmd.exe" ]] || fail "UnrealEditor-Cmd.exe bulunamadı: $UE/Binaries/Win64/UnrealEditor-Cmd.exe"

mkdir -p "$RUNS" "$LOGS"

if command -v tasklist >/dev/null 2>&1 && tasklist 2>/dev/null | grep -qi "UnrealEditor.exe"; then
  fail "Unreal Editor açık. Testlerde aynı DLL'in kullanılması için Editörü kapatıp tekrar çalıştırın."
fi

if [[ "${1:-}" != "--no-build" ]]; then
  rm -f "$ROOT"/Binaries/Win64/UnrealEditor-MyProject2.patch_*
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
fi

# Remove old engine logs so a failed run cannot accidentally copy a stale result.
rm -f "$WOOD_RESULT_LOG" "$MECHANICS_RESULT_LOG"

echo "Wood otomasyon testleri başlıyor..."
if MSYS_NO_PATHCONV=1 timeout 400 "$UE/Binaries/Win64/UnrealEditor-Cmd.exe" "$PROJECT_WIN" \
    -ExecCmds="Automation RunTests MyProject2.Wood; Quit" \
    -unattended -nullrhi -nosplash -nop4 -log=WoodTests.log >/dev/null 2>&1; then
  :
else
  rc=$?
  echo "Wood test süreci başarısız veya zaman aşımına uğradı (exit=$rc)." >&2
  tail -n 100 "$WOOD_RESULT_LOG" >&2 2>/dev/null || true
  exit "$rc"
fi

[[ -s "$WOOD_RESULT_LOG" ]] || fail "Wood sonuç log'u oluşturulmadı: $WOOD_RESULT_LOG"
cp "$WOOD_RESULT_LOG" "$WOOD_SAVED_LOG"

WOOD_PASSES="$(grep -a 'Test Completed' "$WOOD_SAVED_LOG" | grep -c 'Success' || true)"
WOOD_FAILURES="$(grep -ac 'Result={Fail' "$WOOD_SAVED_LOG" || true)"
if [[ "$WOOD_PASSES" -lt 1 ]]; then
  fail "Wood test log'unda başarılı test sonucu bulunamadı; başarı varsayılmadı. Log: $WOOD_SAVED_LOG"
fi
if [[ "$WOOD_FAILURES" -ne 0 ]]; then
  echo "Wood testleri başarısız: $WOOD_FAILURES failure kaydı." >&2
  tail -n 100 "$WOOD_SAVED_LOG" >&2 || true
  exit 1
fi

echo "Mekanik demo testleri başlıyor..."
if MSYS_NO_PATHCONV=1 timeout 600 "$UE/Binaries/Win64/UnrealEditor-Cmd.exe" "$PROJECT_WIN" /Game/Maps/Demo_WoodChain \
    -game -nullrhi -unattended -nosplash -nop4 -log -MechanicsDemoQuit >/dev/null 2>&1; then
  :
else
  rc=$?
  echo "Mekanik demo süreci başarısız veya zaman aşımına uğradı (exit=$rc)." >&2
  tail -n 100 "$MECHANICS_RESULT_LOG" >&2 2>/dev/null || true
  exit "$rc"
fi

[[ -s "$MECHANICS_RESULT_LOG" ]] || fail "Mekanik sonuç log'u oluşturulmadı: $MECHANICS_RESULT_LOG"
cp "$MECHANICS_RESULT_LOG" "$MECHANICS_SAVED_LOG"

MECHANICS_RESULT="$(grep -aE 'SONUÇ: [0-9]+ geçti, [0-9]+ kaldı' "$MECHANICS_SAVED_LOG" | tail -n 1 || true)"
[[ -n "$MECHANICS_RESULT" ]] || fail "Mekanik sonuç satırı yok; test başarılı kabul edilmedi. Log: $MECHANICS_SAVED_LOG"
if [[ "$MECHANICS_RESULT" =~ SONUÇ:[[:space:]]*([0-9]+)[[:space:]]+geçti,[[:space:]]*([0-9]+)[[:space:]]+kaldı ]]; then
  MECHANICS_PASSES="${BASH_REMATCH[1]}"
  MECHANICS_FAILURES="${BASH_REMATCH[2]}"
else
  fail "Mekanik test sonuç satırı çözümlenemedi: $MECHANICS_RESULT"
fi
if [[ "$MECHANICS_PASSES" -lt 1 || "$MECHANICS_FAILURES" -ne 0 ]]; then
  echo "Mekanik testleri geçmedi: $MECHANICS_RESULT" >&2
  tail -n 100 "$MECHANICS_SAVED_LOG" >&2 || true
  exit 1
fi

# Keep recent engine-result logs for reproducibility; old results are removable.
find "$RUNS" -maxdepth 1 -type f \( -name 'mekanik_*.log' -o -name 'wood_*.log' \) -printf '%T@ %p\n' \
  | sort -nr | awk 'NR > 30 { $1=""; sub(/^ /,""); print }' | while IFS= read -r old; do rm -f "$old"; done

python "$ROOT/Scripts/update_docs.py"

echo "GEÇTİ — Mekanik: $MECHANICS_PASSES geçti, $MECHANICS_FAILURES kaldı"
echo "GEÇTİ — Wood: $WOOD_PASSES başarı, $WOOD_FAILURES failure kaydı"
echo "Kayıtlar: $MECHANICS_SAVED_LOG ; $WOOD_SAVED_LOG"
