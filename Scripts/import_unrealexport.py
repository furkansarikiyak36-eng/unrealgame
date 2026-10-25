"""Imports the hand-made log FBX files of do/UnrealExport into /Game/UnrealExport (Archive/ into /Game/UnrealExport/Archive).

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/import_unrealexport.py"
One static mesh per file, its UCX_ mesh as the collision (no generated collision), materials created from the FBX
material names (Bark, Cut_Wood, Hewn_Wood, Peeled_Wood, Peeled_Cut; shared between files), no textures.
Report with each mesh's size in cm and collision hull count: Saved/import_unrealexport.json.
"""
import json
import os

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE = os.path.join(PROJECT, 'do', 'UnrealExport')
DEST = '/Game/UnrealExport'
OUT = os.path.join(PROJECT, 'Saved', 'import_unrealexport.json')


def options():
    ui = unreal.FbxImportUI()
    ui.import_mesh = True
    ui.import_as_skeletal = False
    ui.import_materials = True
    ui.import_textures = False
    ui.import_animations = False
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    data = ui.static_mesh_import_data
    data.combine_meshes = True
    data.auto_generate_collision = False
    data.one_convex_hull_per_ucx = True
    data.generate_lightmap_u_vs = False
    # Blender wrote metre values under a centimetre unit header: the legacy importer brings them in 100 times too
    # small (and at that size welds vertices), Interchange corrects it. Scaled back to real size here.
    data.import_uniform_scale = 100.0
    data.convert_scene_unit = False
    return ui


def main():
    # A re-import over existing assets keeps their old import settings (scale): start from an empty folder.
    if unreal.EditorAssetLibrary.does_directory_exist(DEST):
        unreal.EditorAssetLibrary.delete_directory(DEST)
    tasks = []
    for root, _, files in os.walk(SOURCE):
        for name in sorted(files):
            if not name.lower().endswith('.fbx'):
                continue
            rel = os.path.relpath(root, SOURCE)
            task = unreal.AssetImportTask()
            task.filename = os.path.join(root, name)
            task.destination_path = DEST if rel == '.' else DEST + '/' + rel.replace('\\', '/')
            task.automated = True
            task.replace_existing = True
            task.save = True
            task.options = options()
            tasks.append(task)
    # Interchange (the default FBX path in 5.8) ignores FbxImportUI and drops the UCX_ meshes without making them the
    # collision; the legacy FBX importer turns each UCX_ mesh into a convex collision element. Switched back after.
    flag = 'Interchange.FeatureFlags.Import.FBX'
    before = unreal.SystemLibrary.get_console_variable_int_value(flag)
    unreal.SystemLibrary.execute_console_command(None, flag + ' 0')
    try:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    finally:
        unreal.SystemLibrary.execute_console_command(None, '{} {}'.format(flag, before))
    report = {}
    for task in tasks:
        for path in task.imported_object_paths:
            asset = unreal.load_asset(path)
            if isinstance(asset, unreal.StaticMesh):
                box = asset.get_bounding_box()
                hulls = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).get_convex_collision_count(asset)
                report[path] = {'size': [round(box.max.x - box.min.x, 1), round(box.max.y - box.min.y, 1),
                                         round(box.max.z - box.min.z, 1)],
                                'tris': asset.get_num_triangles(0), 'hulls': hulls,
                                'materials': [str(m.material_slot_name) for m in asset.static_materials]}
    with open(OUT, 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[ImportUnrealExport] {} meshes -> {}'.format(len(report), OUT))


main()
