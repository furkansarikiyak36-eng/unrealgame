"""Saves Main, then deletes the vegetation packs dropped on 2026-10-19 (user approved): Megaplant trees whose every
variant is over 5 M Nanite triangles, the Megaplant plants and shrubs, and PN_OpenWorldFoliage except its
UE4_ThirdPerson folder (the mannequin Olcu_Karakter uses). Plants now come from Megascans only.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/delete_unused_vegetation.py"
Not undoable: these folders are not in git. Report: Saved/delete_unused_vegetation.json.
"""
import json
import os

import unreal

FOLDERS = [
    '/Game/Megaplant_Library/Tree_Black_Alder', '/Game/Megaplant_Library/Tree_Hornbeam',
    '/Game/Megaplant_Library/Tree_Yoshino_Cherry', '/Game/Megaplant_Library/Tree_Black_Poplar',
    '/Game/Megaplant_Library/Plant_Bilberry', '/Game/Megaplant_Library/Plant_Bulrush',
    '/Game/Megaplant_Library/Plant_Dry_Common_Reeds', '/Game/Megaplant_Library/Plant_Lingonberry',
    '/Game/Megaplant_Library/Plant_Small_Balsam', '/Game/Megaplant_Library/Plant_Stinging_Nettle',
    '/Game/Megaplant_Library/Plant_Wood_Anemone',
    '/Game/Megaplant_Library/Shrub_Beach_Rose', '/Game/Megaplant_Library/Shrub_European_Spindle',
    '/Game/Megaplant_Library/Shrub_Greasewood', '/Game/Megaplant_Library/Shrub_Heather',
    '/Game/Megaplant_Library/Shrub_Huckleberry_Oak',
    '/Game/PN_OpenWorldFoliage/ColorExamples', '/Game/PN_OpenWorldFoliage/Maps', '/Game/PN_OpenWorldFoliage/Materials',
    '/Game/PN_OpenWorldFoliage/Meshes', '/Game/PN_OpenWorldFoliage/Textures',
]
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'delete_unused_vegetation.json')


def main():
    report = {'map_saved': unreal.EditorLoadingAndSavingUtils.save_current_level(), 'folders': {}}
    for folder in FOLDERS:
        if not unreal.EditorAssetLibrary.does_directory_exist(folder):
            report['folders'][folder] = 'missing'
            continue
        before = len(unreal.EditorAssetLibrary.list_assets(folder, True, False))
        ok = unreal.EditorAssetLibrary.delete_directory(folder)
        left = unreal.EditorAssetLibrary.list_assets(folder, True, False) if unreal.EditorAssetLibrary.does_directory_exist(folder) else []
        report['folders'][folder] = {'assets': before, 'ok': bool(ok), 'left': [str(a) for a in left][:20]}
    with open(OUT, 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[DeleteVegetation] -> ' + OUT)


main()
