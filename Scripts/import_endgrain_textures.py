"""Imports the end-grain slice textures (do/EndGrain) into /Game/UnrealExport/Textures.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/import_endgrain_textures.py"
Source: do/endgrain_ao.zip (Materialize output of a log-slice photo, no watermark found; licence of the photo not
known). The disc was cut out square (radius = 0.9625 of the half width) and the white background filled with the
bark rim's colour, the normal map with flat normal; both 2048 px. endgrainv2.zip is not used: an Adobe Stock preview
(ID 228416058 on the left edge, watermark pattern). Report: Saved/import_endgrain_textures.json.
"""
import json
import os

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE = os.path.join(PROJECT, 'do', 'EndGrain')
DEST = '/Game/UnrealExport/Textures'
OUT = os.path.join(PROJECT, 'Saved', 'import_endgrain_textures.json')


def main():
    tasks = []
    for name in ('T_EndGrain_Slice_C', 'T_EndGrain_Slice_N'):
        task = unreal.AssetImportTask()
        task.filename = os.path.join(SOURCE, name + '.png')
        task.destination_path = DEST
        task.automated = True
        task.replace_existing = True
        task.save = False
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    report = {}
    for name in ('T_EndGrain_Slice_C', 'T_EndGrain_Slice_N'):
        tex = unreal.load_asset(DEST + '/' + name)
        if name.endswith('_N'):
            tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property('srgb', False)
        tex.set_editor_property('virtual_texture_streaming', False)
        unreal.EditorAssetLibrary.save_loaded_asset(tex, False)
        report[name] = [tex.blueprint_get_size_x(), tex.blueprint_get_size_y(), str(tex.get_editor_property('compression_settings'))]
    with open(OUT, 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[ImportEndGrain] ' + json.dumps(report))


main()
