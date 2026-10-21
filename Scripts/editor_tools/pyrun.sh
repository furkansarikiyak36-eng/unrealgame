#!/bin/sh
# pyrun.sh <script> [args] <expected json in Saved>
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"; ROOT_W="$(cd "$ROOT" && (pwd -W 2>/dev/null || pwd))"
UE_ROOT="${UE_ROOT:-C:/Program Files/Epic Games/UE_5.8}"; UE_ROOT_U="$(cygpath -u "$UE_ROOT" 2>/dev/null || echo "$UE_ROOT")"
cd "$ROOT"
ST=SlateInspectorToolset.SlateInspectorToolset
c(){ timeout 60 python Scripts/ue_mcp.py tools/call "{\"name\":\"call_tool\",\"arguments\":{\"toolset_name\":\"$ST\",\"tool_name\":\"$1\",\"arguments\":$2}}" >/dev/null 2>&1; }
OUTF="Saved/$3"; rm -f "$OUTF"
CMD="py \\\"$ROOT_W/Scripts/$1\\\" $2"
c FillForm "{\"fields\":[{\"ref\":\"${CMDREF:-tb1}\",\"value\":\"$CMD\",\"fieldType\":\"textbox\"}]}"
c Type '{"ref":"'${CMDREF:-tb1}'","text":"","submit":true}'
for i in $(seq 1 ${4:-150}); do [ -f "$OUTF" ] && break; grep -q "$1\", line" <(tail -80 Saved/Logs/MyProject2.log) && break; python -c "import time;time.sleep(4)"; done
tail -80 Saved/Logs/MyProject2.log | grep -A4 "$1\", line" | head -6 | cut -c1-220
[ -f "$OUTF" ] && cat "$OUTF" | head -c 600; echo
