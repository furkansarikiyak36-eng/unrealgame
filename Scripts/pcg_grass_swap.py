"""Swaps the meshes of the Grass Spawner in /Game/PerfTest/PCG_GrassScatter and regenerates GrassScatterVolume.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/pcg_grass_swap.py" <megascans folder>
Default folder Kikuyu_Grass_wiljearqx. Every new entry copies the first old entry's descriptor (shadow, cull,
collision...) and only changes the mesh, equal weights, so the test differs in the mesh alone. The graph is copied
to PCG_GrassScatter_PNBackup once before the first swap. Report: Saved/pcg_grass_swap.json.
"""
import json
import os
import sys

import unreal

GRAPH = '/Game/PerfTest/PCG_GrassScatter'
BACKUP = '/Game/PerfTest/PCG_GrassScatter_PNBackup'

folder = sys.argv[1] if len(sys.argv) > 1 else 'Kikuyu_Grass_wiljearqx'
if not unreal.EditorAssetLibrary.does_asset_exist(BACKUP):
    unreal.EditorAssetLibrary.duplicate_asset(GRAPH, BACKUP)
reg = unreal.AssetRegistryHelpers.get_asset_registry()
meshes = sorted(str(d.package_name) + '.' + str(d.asset_name)
                for d in reg.get_assets_by_path('/Game/Fab/Megascans/Plants/' + folder, recursive=True)
                if str(d.asset_class_path.asset_name) == 'StaticMesh')
graph = unreal.load_asset(GRAPH)
old_meshes = []
for n in graph.get_editor_property('nodes'):
    s = n.get_settings()
    if not isinstance(s, unreal.PCGStaticMeshSpawnerSettings):
        continue
    params = s.get_editor_property('mesh_selector_parameters')
    old = list(params.get_editor_property('mesh_entries'))
    template = old[0].export_text()
    old_path = str(old[0].get_editor_property('descriptor').get_editor_property('static_mesh').get_path_name())
    old_meshes = [str(e.get_editor_property('descriptor').get_editor_property('static_mesh').get_path_name()) for e in old]
    entries = []
    for m in meshes:
        e = unreal.PCGMeshSelectorWeightedEntry()
        e.import_text(template.replace(old_path, m))
        e.set_editor_property('weight', 1)
        entries.append(e)
    params.set_editor_property('mesh_entries', entries)
unreal.EditorAssetLibrary.save_loaded_asset(graph, False)
regen = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    for c in a.get_components_by_class(unreal.PCGComponent):
        if c.get_graph() and c.get_graph().get_path_name().startswith(GRAPH + '.'):
            c.cleanup(True)
            c.generate(True)
            regen += 1
json.dump({'folder': folder, 'new_meshes': meshes, 'old_meshes': len(old_meshes), 'regenerated': regen,
           'backup': BACKUP}, open(os.path.join(unreal.Paths.project_saved_dir(), 'pcg_grass_swap.json'), 'w'), indent=1)
unreal.log('[PCGGrassSwap] {} meshes from {}, {} components regenerated'.format(len(meshes), folder, regen))
