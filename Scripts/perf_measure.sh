#!/bin/sh
# Measures the open level from fixed camera poses with the CSV profiler; appends to Saved/perftest_results.json.
# usage: Scripts/perf_measure.sh <label>
cd "$(dirname "$0")/.."
LABEL="$1"; ST=SlateInspectorToolset.SlateInspectorToolset
c(){ timeout 60 python Scripts/ue_mcp.py tools/call "{\"name\":\"call_tool\",\"arguments\":{\"toolset_name\":\"$ST\",\"tool_name\":\"$1\",\"arguments\":$2}}" >/dev/null 2>&1; }
cmd(){ c FillForm '{"fields":[{"ref":"tb2","value":"","fieldType":"textbox"}]}'; c Click '{"ref":"tb2","button":"left","doubleClick":false}'; c Type "{\"ref\":\"tb2\",\"text\":\"$1\",\"submit\":true}"; }
cam(){ timeout 60 python Scripts/ue_mcp.py tools/call "{\"name\":\"call_tool\",\"arguments\":{\"toolset_name\":\"EditorToolset.EditorAppToolset\",\"tool_name\":\"SetCameraTransform\",\"arguments\":{\"transform\":{\"location\":$1,\"rotation\":$2,\"scale\":{\"x\":1,\"y\":1,\"z\":1}}}}}" >/dev/null 2>&1; }
for pose in ground air; do
  if [ $pose = ground ]; then cam '{"x":0,"y":0,"z":170}' '{"pitch":-3,"yaw":35,"roll":0}'; else cam '{"x":-12000,"y":-12000,"z":15000}' '{"pitch":-30,"yaw":45,"roll":0}'; fi
  python -c "import time;time.sleep(5)"
  ended=$(grep -c 'Capture Ended' Saved/Logs/MyProject2.log)
  cmd 'csvprofile start'; python -c "import time;time.sleep(6)"; cmd 'csvprofile stop'
  # The file exists from the start of the capture and stays locked until the profiler logs 'Capture Ended'.
  for i in $(seq 1 30); do [ $(grep -c 'Capture Ended' Saved/Logs/MyProject2.log) -gt $ended ] && break; python -c "import time;time.sleep(1)"; done
  python -c "import time;time.sleep(1)"
  python Scripts/perf_parse.py "$LABEL" "$pose" "$(ls -t Saved/Profiling/CSV/*.csv | head -1)"
done
