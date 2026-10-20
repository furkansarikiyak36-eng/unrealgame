import json,subprocess,sys,base64
out,x,y,z,p,yw=sys.argv[1],*map(float,sys.argv[2:7])
a={"name":"call_tool","arguments":{"toolset_name":"EditorToolset.EditorAppToolset","tool_name":"CaptureViewport","arguments":{"captureTransform":{"location":{"x":x,"y":y,"z":z},"rotation":{"pitch":p,"yaw":yw,"roll":0},"scale":{"x":1,"y":1,"z":1}},"annotations":[],"bShowUI":False}}}
r=json.loads(subprocess.run(["python","Scripts/ue_mcp.py","tools/call",json.dumps(a)],capture_output=True,text=True,timeout=120).stdout)['result']['content']
v=json.loads(r[0]['text'])['returnValue']['image']
open(out,'wb').write(base64.b64decode(v['data'])); print('ok',out)
