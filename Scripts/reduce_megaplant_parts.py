"""Lowers the Nanite triangle count of the Megaplant branch parts as far as it cannot be seen from 2 m.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/reduce_megaplant_parts.py"
Nanite draws about one triangle per pixel, so a part never needs more triangles than the pixels it covers. For every
Branch/Twig/Bark/Decoration part of the packs below: pixels of its largest face seen from 2 m on a 1920 px wide,
90 degree view; Keep Triangles = that / its Nanite triangles, rounded up to 0.05, at least 0.05. Parts that would
keep 90 % or more, and leaves, needles and flowers, are left alone. Every changed asset is copied to
Saved/Backups/2026-10-19/nanite/ first (restore: copy back, or run with RESTORE = True). Report:
Saved/reduce_megaplant_parts.json.
"""
import json
import math
import os
import shutil

import unreal

RESTORE = False
PACKS = ['Tree_Baltic_Pine', 'Tree_Dead_Pine', 'Tree_Rowan', 'Tree_Norway_Maple', 'Tree_Japanese_Cypress',
         'Tree_European_Aspen', 'Plant_Bilberry', 'Plant_Bulrush', 'Plant_Dry_Common_Reeds', 'Plant_Lingonberry',
         'Plant_Small_Balsam', 'Plant_Stinging_Nettle', 'Plant_Wood_Anemone', 'Shrub_Huckleberry_Oak']
KINDS = ('branch', 'twig', 'bark', 'decoration', 'top', 'stem')
SKIP = ('leaf', 'needle', 'flower', 'berry')
DISTANCE_CM = 200.0
SCREEN_PX = 1920.0
PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
BACKUP = os.path.join(PROJECT, 'Saved', 'Backups', '2026-10-19', 'nanite')
OUT = os.path.join(PROJECT, 'Saved', 'reduce_megaplant_parts.json')


def disk_path(package):
    return os.path.join(PROJECT, 'Content', package[len('/Game/'):] + '.uasset')


def main():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    px_per_cm = (SCREEN_PX / 2.0) / DISTANCE_CM
    report = []
    for pack in PACKS:
        for data in registry.get_assets_by_path('/Game/Megaplant_Library/' + pack + '/Instances', recursive=True):
            if str(data.asset_class_path.asset_name) != 'StaticMesh':
                continue
            package = str(data.package_name)
            name = package.rsplit('/', 1)[-1].lower()
            if any(s in name for s in SKIP) or not any(k in name for k in KINDS):
                continue
            mesh = unreal.load_asset(package)
            settings = sub.get_nanite_settings(mesh)
            tris = mesh.get_num_nanite_triangles()
            if RESTORE:
                keep = 1.0
            else:
                box = mesh.get_bounding_box()
                dims = sorted([box.max.x - box.min.x, box.max.y - box.min.y, box.max.z - box.min.z])
                pixels = dims[1] * dims[2] * px_per_cm * px_per_cm
                keep = min(1.0, max(0.05, math.ceil(pixels / max(tris, 1) / 0.05) * 0.05))
                if keep >= 0.9:
                    continue
            if abs(settings.keep_percent_triangles - keep) < 1e-3:
                continue
            if not RESTORE:
                os.makedirs(BACKUP, exist_ok=True)
                target = os.path.join(BACKUP, pack + '__' + package.rsplit('/', 1)[-1] + '.uasset')
                if not os.path.exists(target):
                    shutil.copy2(disk_path(package), target)
            settings.keep_percent_triangles = keep
            sub.set_nanite_settings(mesh, settings, True)
            unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
            report.append({'pack': pack, 'part': package.rsplit('/', 1)[-1], 'tris_before': tris, 'keep': keep,
                           'tris_after': mesh.get_num_nanite_triangles()})
    with open(OUT, 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[ReduceMegaplant] {} parts -> {}'.format(len(report), OUT))


main()
