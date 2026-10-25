"""Read only: weight of the Megaplant tree assets. Writes Saved/measure_trees.json.

Per pack: every static mesh part (Nanite triangles and vertices, fallback triangles, Nanite keep/trim settings),
the trunk skeletal meshes (vertices of LOD 0, the Nanite assembly part count) and every texture (size).
Run in the editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/measure_trees.py"
"""
import json
import os

import unreal

OUT = os.path.join(unreal.Paths.project_saved_dir(), 'measure_trees.json')
ROOT = '/Game/Megaplant_Library'


def main():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    sm_sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    sk_sub = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
    report = {}
    for pack in registry.get_sub_paths(ROOT, False):
        name = str(pack).rsplit('/', 1)[-1]
        entry = {'parts': {}, 'trees': {}, 'textures': {}}
        for data in registry.get_assets_by_path(pack, recursive=True):
            cls = str(data.asset_class_path.asset_name)
            path = str(data.package_name)
            short = path.rsplit('/', 1)[-1]
            if cls == 'StaticMesh':
                mesh = unreal.load_asset(path)
                ns = sm_sub.get_nanite_settings(mesh)
                entry['parts'][short] = {
                    'nanite_tris': mesh.get_num_nanite_triangles(),
                    'nanite_verts': mesh.get_num_nanite_vertices(),
                    'fallback_tris': mesh.get_num_triangles(0),
                    'keep': round(ns.keep_percent_triangles, 3),
                    'trim': round(ns.trim_relative_error, 4),
                }
            elif cls == 'SkeletalMesh':
                mesh = unreal.load_asset(path)
                item = {'verts0': sk_sub.get_num_verts(mesh, 0), 'lods': sk_sub.get_lod_count(mesh)}
                try:
                    ns = mesh.get_editor_property('nanite_settings')
                    item['nanite'] = bool(ns.get_editor_property('enabled'))
                    item['assembly_parts'] = len(ns.get_editor_property('nanite_assembly_data').get_editor_property('parts'))
                except Exception as error:
                    item['nanite_error'] = str(error)[:120]
                entry['trees'][short] = item
            elif cls.startswith('Texture2D'):
                tex = unreal.load_asset(path)
                entry['textures'][short] = [tex.blueprint_get_size_x(), tex.blueprint_get_size_y()]
        report[name] = entry
    with open(OUT, 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[MeasureTrees] wrote ' + OUT)


main()
