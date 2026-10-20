"""Bounding boxes of the UnrealExport raw logs: Saved/rawlog_dims.json"""
import json, os
import unreal
out = {}
for n in ('SM_RawLog_600', 'SM_RawLog_300', 'SM_RawLog_150', 'SM_RawLog_600_Half', 'SM_RawLog_300_Half'):
    m = unreal.load_asset('/Game/UnrealExport/' + n)
    if m:
        b = m.get_bounding_box()
        out[n] = {'min': [round(b.min.x), round(b.min.y), round(b.min.z)], 'max': [round(b.max.x), round(b.max.y), round(b.max.z)],
                  'collision_prims': m.get_editor_property('body_setup').get_editor_property('agg_geom').export_text()[:120] if m.get_editor_property('body_setup') else None}
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'rawlog_dims.json'), 'w'), indent=1)
