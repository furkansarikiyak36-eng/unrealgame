#!/bin/bash
# Mekanik testi ve Wood birim testlerini çalıştırır, kayıtları Saved/TestRuns altına saklar, dokümanları günceller.
# Editör KAPALI olmalı (derleme ve testler aynı DLL'i kullanır).
#
#   bash Scripts/run_tests.sh            # derle + iki test + dokümanlar
#   bash Scripts/run_tests.sh --no-build # derlemeden
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE="/c/Program Files/Epic Games/UE_5.8/Engine"
PROJECT_WIN="$(cygpath -w "$ROOT/MyProject2.uproject")"
STAMP="$(date +%Y-%m-%d_%H%M)"
mkdir -p "$ROOT/Saved/TestRuns"

if tasklist 2>/dev/null | grep -qi "UnrealEditor.exe"; then
  echo "Editör açık: kapatıp yeniden çalıştırın." >&2
  exit 2
fi

if [ "${1:-}" != "--no-build" ]; then
  rm -f "$ROOT"/Binaries/Win64/UnrealEditor-MyProject2.patch_*
  "$UE/Build/BatchFiles/Build.bat" MyProject2Editor Win64 Development -Project="$PROJECT_WIN" -WaitMutex 2>&1 | grep -E "error|Result"
fi

MSYS_NO_PATHCONV=1 timeout 400 "$UE/Binaries/Win64/UnrealEditor-Cmd.exe" "$PROJECT_WIN" \
  -ExecCmds="Automation RunTests MyProject2.Wood; Quit" -unattended -nullrhi -nosplash -nop4 -log=WoodTests.log > /dev/null 2>&1
cp "$ROOT/Saved/Logs/WoodTests.log" "$ROOT/Saved/TestRuns/wood_$STAMP.log"

MSYS_NO_PATHCONV=1 timeout 600 "$UE/Binaries/Win64/UnrealEditor-Cmd.exe" "$PROJECT_WIN" /Game/Maps/Demo_WoodChain \
  -game -nullrhi -unattended -nosplash -nop4 -log -MechanicsDemoQuit > /dev/null 2>&1
cp "$ROOT/Saved/Logs/MyProject2.log" "$ROOT/Saved/TestRuns/mekanik_$STAMP.log"

# Eski kayıtlardan yalnızca son 15'i kalsın
ls -1t "$ROOT"/Saved/TestRuns/mekanik_*.log 2>/dev/null | tail -n +16 | xargs -r rm -f
ls -1t "$ROOT"/Saved/TestRuns/wood_*.log 2>/dev/null | tail -n +16 | xargs -r rm -f

python "$ROOT/Scripts/update_docs.py"
grep -a "SONUÇ" "$ROOT/Saved/TestRuns/mekanik_$STAMP.log" | tail -1 | cut -c60-
echo "Wood: $(grep -a 'Test Completed' "$ROOT/Saved/TestRuns/wood_$STAMP.log" | grep -c Success) geçti, $(grep -a 'Result={Fail' "$ROOT/Saved/TestRuns/wood_$STAMP.log" | wc -l) kaldı"
