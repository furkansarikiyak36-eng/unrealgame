"""Read-only: texture settings and a PNG export of each given texture.
py ".../probe_textures.py" /Game/t1,/Game/t2 -> Saved/probe_textures.json, Saved/TexExport/<name>.png"""
import json, os, sys
import unreal
out = {}; ex = os.path.join(unreal.Paths.project_saved_dir(), 'TexExport'); os.makedirs(ex, exist_ok=True)
for p in sys.argv[1].split(','):
    t = unreal.load_asset(p)
    if not t:
        out[p] = 'missing'; continue
    r = {}
    for k in ('compression_settings', 'srgb', 'max_texture_size', 'lod_group', 'lod_bias', 'mip_gen_settings', 'never_stream',
              'compression_no_alpha', 'virtual_texture_streaming', 'filter', 'address_x', 'address_y'):
        try: r[k] = str(t.get_editor_property(k))
        except Exception as e: r[k] = 'n/a'
    r['size'] = [t.blueprint_get_size_x(), t.blueprint_get_size_y()]
    task = unreal.AssetExportTask(); task.object = t; task.filename = os.path.join(ex, p.split('/')[-1] + '.png')
    task.automated = True; task.replace_identical = True; task.prompt = False
    r['exported'] = unreal.Exporter.run_asset_export_task(task)
    out[p] = r
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'probe_textures.json'), 'w'), indent=1)
