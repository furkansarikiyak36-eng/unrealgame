# measure_view.sh <label> x y z pitch yaw : CSV capture from one camera pose, appended to perftest_results.json
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"; ROOT_W="$(cd "$ROOT" && (pwd -W 2>/dev/null || pwd))"
UE_ROOT="${UE_ROOT:-C:/Program Files/Epic Games/UE_5.8}"; UE_ROOT_U="$(cygpath -u "$UE_ROOT" 2>/dev/null || echo "$UE_ROOT")"
cd "$ROOT"
ST=SlateInspectorToolset.SlateInspectorToolset
c(){ timeout 60 python Scripts/ue_mcp.py tools/call "{\"name\":\"call_tool\",\"arguments\":{\"toolset_name\":\"$ST\",\"tool_name\":\"$1\",\"arguments\":$2}}" >/dev/null 2>&1; }
cmd(){ c FillForm "{\"fields\":[{\"ref\":\"${CMDREF:-tb1}\",\"value\":\"$1\",\"fieldType\":\"textbox\"}]}"; c Type '{"ref":"'${CMDREF:-tb1}'","text":"","submit":true}'; }
timeout 60 python Scripts/ue_mcp.py tools/call "{\"name\":\"call_tool\",\"arguments\":{\"toolset_name\":\"EditorToolset.EditorAppToolset\",\"tool_name\":\"SetCameraTransform\",\"arguments\":{\"transform\":{\"location\":{\"x\":$2,\"y\":$3,\"z\":$4},\"rotation\":{\"pitch\":$5,\"yaw\":$6,\"roll\":0},\"scale\":{\"x\":1,\"y\":1,\"z\":1}}}}}" >/dev/null 2>&1
python -c "import time;time.sleep(8)"
ended=$(grep -c 'Capture Ended' Saved/Logs/MyProject2.log)
cmd 'csvprofile start'; python -c "import time;time.sleep(6)"; cmd 'csvprofile stop'
for i in $(seq 1 30); do [ $(grep -c 'Capture Ended' Saved/Logs/MyProject2.log) -gt $ended ] && break; python -c "import time;time.sleep(1)"; done; python -c "import time;time.sleep(1)"
python Scripts/perf_parse.py "$1" view "$(ls -t Saved/Profiling/CSV/*.csv | head -1)" | python -c "
import sys,json;r=json.loads(sys.stdin.read());print(r['label'],{k:r.get(k) for k in ['FrameTime','RenderThreadTime','GPUTime','GPUMem/LocalUsedMB']})"
