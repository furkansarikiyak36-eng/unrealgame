import os
import json, subprocess
a = {"name": "call_tool", "arguments": {"toolset_name": "SlateInspectorToolset.SlateInspectorToolset", "tool_name": "Snapshot", "arguments": {"ref": "", "maxDepth": 60}}}
r = subprocess.run(["python", "Scripts/ue_mcp.py", "tools/call", json.dumps(a)], capture_output=True, text=True, timeout=90, cwd=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")).stdout
t = json.loads(json.loads(r)['result']['content'][0]['text'])['returnValue']
cand = [l for l in t.splitlines() if l.strip().startswith('textbox') and '"' not in l.split('[pos=')[0]]
print('CMDREF', cand[-1].split('[ref=')[1].rstrip(']') if cand else 'NOTFOUND')
