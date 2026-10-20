"""Read only: compile errors of M_RawLog. Writes Saved/probe_compile.json."""
import json, os
import unreal
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'probe_compile.json')
lib = unreal.MaterialEditingLibrary
m = unreal.load_asset('/Game/UnrealExport/Materials/M_RawLog')
rep = {'lib': [n for n in dir(lib) if 'rror' in n or 'ompil' in n or 'tat' in n.lower()],
       'mat': [n for n in dir(m) if 'rror' in n or 'ompil' in n]}
for name in rep['lib']:
    try:
        rep[name] = str(getattr(lib, name)(m))[:2000]
    except Exception as e:
        rep[name] = 'ERR ' + str(e)[:200]
json.dump(rep, open(OUT, 'w'), indent=1)
unreal.log('[ProbeCompile] done')
