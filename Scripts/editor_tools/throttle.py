import json,subprocess
args={"name":"call_tool","arguments":{"toolset_name":"editor_toolset.toolsets.object.ObjectTools","tool_name":"set_properties","arguments":{"instance":{"refPath":"/Script/UnrealEd.Default__EditorPerformanceSettings"},"values":json.dumps({"bThrottleCPUWhenNotForeground":False})}}}
print(subprocess.run(["python","Scripts/ue_mcp.py","tools/call",json.dumps(args)],capture_output=True,text=True).stdout[:300])
