"""Turns the FieldGrass PCG test into a mixed meadow: FieldGrass plus light flowers, ground plants, mushrooms and bushes,
as weighted entries of the one mesh spawner in /Game/PerfTest/PCG_FieldGrass. Per entry: cull distance, cast shadow;
for all: no wind (WPO off), outside Lumen / distance-field lighting. Only variants up to MAX_TRIS triangles are used
(Saved/asset_bundles.json). Total density is raised so grass keeps ~4/m2. Regenerates and saves PerfTest.
py ".../set_meadow_mix.py" [total density=4.6] -> Saved/meadow_mix.json"""
import json, os, sys
import unreal
GRAPH = '/Game/PerfTest/PCG_FieldGrass'
DENSITY = float(sys.argv[1]) if len(sys.argv) > 1 else 4.6
SAVED = unreal.Paths.project_saved_dir()
TRIS = {b['pkg']: (b['ntris'] or b['tris']) if b['nanite'] else b['tris'] for b in json.load(open(os.path.join(SAVED, 'asset_bundles.json'), encoding='utf-8'))}
MS, KD = '/Game/Fab/Megascans/', '/Game/KiteDemo/Environments/'
# (name, folder or mesh, family weight %, max tris, cull end m, shadow)
MIX = [
    ('FieldGrass', KD + 'Foliage/Grass/FieldGrass/SM_FieldGrass_01', 85.75, 99999, 80, False),
    ('Buttercup', KD + 'Foliage/Flowers/Buttercup/SM_Buttercup_Patch_01', 4.0, 99999, 45, False),
    ('Lily of the Valley', MS + 'Plants/Lily_of_the_Valley_xikkdhjja', 2.5, 600, 40, False),
    ('Ground Cover', MS + 'Plants/Ground_Cover_sfcnq', 2.5, 600, 40, False),
    ('Heather', KD + 'Foliage/Flowers/Heather/SM_Heather_Mesh_Clumps2', 0.75, 99999, 80, True),
    ('Dead Shrubs', MS + 'Plants/Dead_Shrubs_vdknafgha', 1.0, 800, 80, True),
    ('Puffball', MS + '3D/Puffball_Mushrooms_qdzs3', 1.25, 99999, 25, True),
    ('Bolete', MS + '3D/Bolete_Mushrooms_qdzrT', 1.25, 99999, 25, True),
    ('Small Stones A', MS + '3D/Small_Stones_Pack_vcljbb1iw', 0.5, 99999, 40, True),
    ('Small Stones B', MS + '3D/Small_Stones_Pack_vdjkbfmiw', 0.2, 99999, 40, True),
    ('Mossy Stones', MS + '3D/Mossy_Stones_Pack_tliiadmva', 0.3, 99999, 40, True),
]
reg = unreal.AssetRegistryHelpers.get_asset_registry()
def meshes_of(path, max_tris):
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return [path]
    out = []
    for d in reg.get_assets_by_path(path, recursive=True):
        p = str(d.package_name)
        if str(d.asset_class_path.asset_name) == 'StaticMesh' and TRIS.get(p, 0) <= max_tris:
            out.append(p)
    return sorted(out)
graph = unreal.load_asset(GRAPH)
rep = {'density': DENSITY, 'families': []}
for n in graph.get_editor_property('nodes'):
    s = n.get_settings()
    if isinstance(s, unreal.PCGSurfaceSamplerSettings):
        s.set_editor_property('points_per_squared_meter', DENSITY)
    if not isinstance(s, unreal.PCGStaticMeshSpawnerSettings):
        continue
    params = s.get_editor_property('mesh_selector_parameters')
    old = list(params.get_editor_property('mesh_entries'))
    tpl = old[0].export_text()
    old_path = str(old[0].get_editor_property('descriptor').get_editor_property('static_mesh').get_path_name())
    entries = []
    for name, path, w, max_tris, cull_m, shadow in MIX:
        ms = meshes_of(path, max_tris)
        if not ms:
            rep['families'].append([name, 'NOT FOUND', path]); continue
        for m in ms:
            obj = m + '.' + m.split('/')[-1]
            e = unreal.PCGMeshSelectorWeightedEntry(); e.import_text(tpl.replace(old_path, obj))
            e.set_editor_property('weight', max(1, int(round(w * 100 / len(ms)))))
            d = e.get_editor_property('descriptor')
            for k, v in (('cast_shadow', shadow), ('evaluate_world_position_offset', False), ('affect_dynamic_indirect_lighting', False),
                         ('affect_distance_field_lighting', False), ('instance_start_cull_distance', int(cull_m * 80)),
                         ('instance_end_cull_distance', int(cull_m * 100))):
                try:
                    d.set_editor_property(k, v)
                except Exception as ex:
                    rep.setdefault('errors', []).append('{} {}: {}'.format(name, k, str(ex)[:60]))
            e.set_editor_property('descriptor', d)
            entries.append(e)
        rep['families'].append([name, len(ms), [TRIS.get(m) for m in ms]])
    params.set_editor_property('mesh_entries', entries)
unreal.EditorAssetLibrary.save_loaded_asset(graph, False)
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    for c in a.get_components_by_class(unreal.PCGComponent):
        if c.get_graph() and c.get_graph().get_path_name().startswith(GRAPH + '.'):
            c.cleanup(True); c.generate(True)
json.dump(rep, open(os.path.join(SAVED, 'meadow_mix.json'), 'w'), indent=1)
