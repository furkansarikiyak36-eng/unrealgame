# rebuild_restart.sh : save + close the editor, build MyProject2Editor, reopen PerfTest, bring the window up, print Cmd box ref
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"; ROOT_W="$(cd "$ROOT" && (pwd -W 2>/dev/null || pwd))"
UE_ROOT="${UE_ROOT:-C:/Program Files/Epic Games/UE_5.8}"; UE_ROOT_U="$(cygpath -u "$UE_ROOT" 2>/dev/null || echo "$UE_ROOT")"
cd "$ROOT"
U="$ROOT/Scripts/editor_tools"
timeout 60 python Scripts/ue_mcp.py tools/call '{"name":"call_tool","arguments":{"toolset_name":"EditorToolset.EditorAppToolset","tool_name":"StopPIE","arguments":{}}}' >/dev/null 2>&1
CMDREF=${CMDREF:-$(python $U/find_cmdref.py | cut -d" " -f2)} bash $U/pyrun.sh save_and_quit_editor.py "" none.json 1 >/dev/null 2>&1
for i in $(seq 1 40); do powershell -NoProfile -Command "if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) {exit 1} else {exit 0}" && break; python -c "import time;time.sleep(3)"; done
powershell -NoProfile -Command "if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) {'EDITOR STILL RUNNING'; exit 1}" || exit 1
"$UE_ROOT_U/Engine/Build/BatchFiles/Build.bat" MyProject2Editor Win64 Development "-Project=$ROOT_W/MyProject2.uproject" -WaitMutex -NoHotReloadFromIDE > $U/build_last.log 2>&1
grep -E "error |error C|Result:" $U/build_last.log | head -20
grep -q "Result: Succeeded" $U/build_last.log || { echo BUILD FAILED; exit 2; }
powershell -NoProfile -Command "Start-Process -FilePath '$UE_ROOT\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList '\"$ROOT_W\MyProject2.uproject\"','/Game/Maps/PerfTest'"
L=$(wc -l < Saved/Logs/MyProject2.log 2>/dev/null || echo 0)
python -c "import time;time.sleep(20)"
for i in $(seq 1 40); do grep -q "Total Editor Startup" Saved/Logs/MyProject2.log && break; python -c "import time;time.sleep(4)"; done
powershell -NoProfile -Command "Add-Type -Name W6 -Namespace U6 -MemberDefinition '[DllImport(\"user32.dll\")] public static extern bool ShowWindowAsync(System.IntPtr h, int n); [DllImport(\"user32.dll\")] public static extern bool SetForegroundWindow(System.IntPtr h);'; \$h=(Get-Process UnrealEditor).MainWindowHandle; [U6.W6]::ShowWindowAsync(\$h,9)|Out-Null; Start-Sleep 2; [U6.W6]::SetForegroundWindow(\$h)|Out-Null"
python -c "import time;time.sleep(6)"
python $U/find_cmdref.py
