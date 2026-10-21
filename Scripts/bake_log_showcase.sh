#!/bin/sh
# bake_log_showcase.sh : builds every LogShowcase station once in play with the real placement, then bakes the result
# into /Game/Maps/LogShowcase as static actors (no building on play). Editor open; CMDREF = Cmd box ref (find_cmdref.py).
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
U="$ROOT/Scripts/editor_tools"
CMDREF=${CMDREF:-$(python $U/find_cmdref.py | cut -d" " -f2)}
a(){ timeout 120 python Scripts/ue_mcp.py tools/call "{\"name\":\"call_tool\",\"arguments\":{\"toolset_name\":\"EditorToolset.EditorAppToolset\",\"tool_name\":\"$1\",\"arguments\":$2}}" >/dev/null 2>&1; }
# The map's own floor at z=0 and no voxel ground first, so the build in play stands on what the editor shows.
CMDREF=$CMDREF bash $U/pyrun.sh make_log_showcase_map.py "" make_log_showcase_map.json 60 >/dev/null
CMDREF=$CMDREF bash $U/pyrun.sh bake_showcase_place.py "" bake_showcase_place.json 60 >/dev/null
rm -f Saved/bake_showcase.json
a StartPIE '{"options":{"bSimulate":false}}'; python -c "import time;time.sleep(15)"
CMDREF=$CMDREF bash $U/pyrun.sh bake_showcase_capture.py "" bake_showcase.json 120 >/dev/null
a StopPIE '{}'; python -c "import time;time.sleep(4)"
CMDREF=$CMDREF bash $U/pyrun.sh bake_showcase_place.py "" bake_showcase_place.json 120 >/dev/null
python -c "
import json
c=json.load(open('Saved/bake_showcase.json',encoding='utf-8')); p=json.load(open('Saved/bake_showcase_place.json'))
print('captured: logs', len(c['logs']), 'assets', c['assets'], 'parts', len(c['parts']), 'signs', len(c['signs']), 'failed', len(c.get('failed', [])))
print('placed:', {k: p.get(k) for k in ('map', 'removed', 'logs', 'parts', 'signs', 'floor_z', 'saved', 'error')})
print([l for l in (c.get('report') or '').splitlines() if 'XX' in l or 'Toplam' in l])"
