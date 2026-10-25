"""Removes the Project Nature (PN_*) kits, as the user asked on 2026-10-20, after relocate_pn_dependencies.py moved the
assets our own content uses. Also removes the PN-based backup of the PCG grass graph.
Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/delete_pn_kits.py"
Report: Saved/delete_pn_kits.json
"""
import json
import os

import unreal

lib = unreal.EditorAssetLibrary
FOLDERS = ['/Game/PN_Fern', '/Game/PN_GrassLibrary', '/Game/PN_MeadowFlowers', '/Game/PN_OpenWorldFoliage', '/Game/PN_WildBerries',
           '/Game/PN_coniferBushes_I', '/Game/PN_tropicalGroundPlants']
report = {'deleted': [], 'failed': []}
if lib.does_asset_exist('/Game/PerfTest/PCG_GrassScatter_PNBackup'):
    (report['deleted'] if lib.delete_asset('/Game/PerfTest/PCG_GrassScatter_PNBackup') else report['failed']).append('PCG_GrassScatter_PNBackup')
for f in FOLDERS:
    if lib.does_directory_exist(f):
        (report['deleted'] if lib.delete_directory(f) else report['failed']).append(f)
json.dump(report, open(os.path.join(unreal.Paths.project_saved_dir(), 'delete_pn_kits.json'), 'w'), indent=1)
unreal.log('[DeletePN] ' + json.dumps(report))
