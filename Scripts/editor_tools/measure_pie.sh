# measure_pie.sh <label> : starts PIE, CSV capture + ProfileGPU, stops PIE, prints medians and top GPU passes
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"; ROOT_W="$(cd "$ROOT" && (pwd -W 2>/dev/null || pwd))"
UE_ROOT="${UE_ROOT:-C:/Program Files/Epic Games/UE_5.8}"; UE_ROOT_U="$(cygpath -u "$UE_ROOT" 2>/dev/null || echo "$UE_ROOT")"
cd "$ROOT"
ST=SlateInspectorToolset.SlateInspectorToolset
c(){ timeout 60 python Scripts/ue_mcp.py tools/call "{\"name\":\"call_tool\",\"arguments\":{\"toolset_name\":\"$ST\",\"tool_name\":\"$1\",\"arguments\":$2}}" >/dev/null 2>&1; }
cmd(){ c FillForm "{\"fields\":[{\"ref\":\"${CMDREF:-tb1}\",\"value\":\"$1\",\"fieldType\":\"textbox\"}]}"; c Type '{"ref":"'${CMDREF:-tb1}'","text":"","submit":true}'; }
a(){ timeout 120 python Scripts/ue_mcp.py tools/call "{\"name\":\"call_tool\",\"arguments\":{\"toolset_name\":\"EditorToolset.EditorAppToolset\",\"tool_name\":\"$1\",\"arguments\":$2}}" >/dev/null 2>&1; }
a StartPIE '{"options":{"bSimulate":false}}'; python -c "import time;time.sleep(15)"
e=$(grep -c 'Capture Ended' Saved/Logs/MyProject2.log); cmd "csvprofile start"; python -c "import time;time.sleep(8)"; cmd "csvprofile stop"; python -c "import time;time.sleep(3)"; cmd "csvprofile stop"
for i in $(seq 1 30); do [ $(grep -c 'Capture Ended' Saved/Logs/MyProject2.log) -gt $e ] && break; python -c "import time;time.sleep(1)"; done; python -c "import time;time.sleep(1)"
python Scripts/perf_parse.py "$1" pie "$(ls -t Saved/Profiling/CSV/*.csv | head -1)" | python -c "
import sys,json;r=json.loads(sys.stdin.read());print(r['label'],{k:r.get(k) for k in ['frames','FrameTime','GameThreadTime','GPUTime','RHI/PrimitivesDrawn','GPUSceneInstanceCount']})"
cmd 'ProfileGPU'; python -c "import time;time.sleep(8)"
a StopPIE '{}'; python -c "import time;time.sleep(3)"; c Click '{"ref":"t2","button":"left","doubleClick":false}'
PYTHONIOENCODING=utf-8 python - <<'PY'
import re
L=open('Saved/Logs/MyProject2.log',encoding='utf-8',errors='ignore').read().splitlines()
s=max(i for i,l in enumerate(L) if 'Cmd: ProfileGPU' in l)
want=('Scene','LumenSceneUpdate','LumenSceneLighting','DiffuseIndirectAndAO','ShadowDepths','BasePass','PrePass','Lights','Translucency','PostProcessing','RenderCloud','CaptureConvolveSkyEnvMap','VolumetricFog','Reflections','Nanite::VisBuffer','RenderVelocities')
seen={}
for l in L[s:s+800]:
    if 'LogRHI: Display:' not in l or '┃' not in l: continue
    m=re.findall(r'([0-9]+\.[0-9]+) ms',l)
    if not m: continue
    name=l.split('┃')[-2].strip() if l.rstrip().endswith('┃') else l.split('┃')[-1].strip()
    for w in want:
        if name.startswith(w) and w not in seen:
            seen[w]=(float(m[-1]),name[:60])
for k,v in sorted(seen.items(),key=lambda x:-x[1][0]): print(' ',v[0],'ms',v[1])
PY
