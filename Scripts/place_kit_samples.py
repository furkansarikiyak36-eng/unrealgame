"""Places one sample of each kind from KiteDemo, ParagonProps and SoulCave in a row 60 m north of the PerfTest grass area,
for before/after texture comparisons.  py ".../place_kit_samples.py"
Replaces the previous row (tag KitSamples), saves the map. Report: Saved/kit_samples.json (labels, positions, sizes).
"""
import json
import os

import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))

TAG = 'KitSamples'
WANT = [  # (kit folder, mesh name)
    ('KiteDemo', 'ScotsPine_01'), ('KiteDemo', 'HillTree_02'), ('KiteDemo', 'LargeVolcanicRock_001'), ('KiteDemo', 'BogMyrtleBush_02'),
    ('ParagonProps', 'Aclove_Wall_Broken'), ('ParagonProps', 'Dusk_Spire_Gate_Pillar_A'), ('ParagonProps', 'RuinRemains5'),
    ('ParagonProps', 'SM_RockNordic_04'), ('ParagonProps', 'Tri_Rock_1'),
    ('SoulCave', 'SM_Cave_Pillar_NoFlag'), ('SoulCave', 'SM_S_Soul_Statue'), ('SoulCave', 'SM_Rocks_Flat_Top'),
]
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
reg = unreal.AssetRegistryHelpers.get_asset_registry()
for a in actors.get_all_level_actors():
    if TAG in [str(t) for t in a.tags]:
        actors.destroy_actor(a)
by_name = {}
for kit in {k for k, _ in WANT}:
    for d in reg.get_assets_by_path('/Game/' + kit, recursive=True):
        if str(d.asset_class_path.asset_name) == 'StaticMesh':
            by_name.setdefault((kit, str(d.asset_name)), str(d.package_name))
x = -2500.0
out = []
for kit, name in WANT:
    pkg = by_name.get((kit, name))
    if not pkg:
        out.append({'mesh': name, 'missing': True})
        continue
    mesh = unreal.load_asset(pkg)
    b = mesh.get_bounding_box()
    w = max(b.max.x - b.min.x, 100.0)
    x += w * 0.5 + 150.0
    loc = unreal.Vector(x - (b.min.x + b.max.x) * 0.5, 6000.0 - (b.min.y + b.max.y) * 0.5, -b.min.z)
    act = actors.spawn_actor_from_object(mesh, loc, unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
    act.set_actor_label('Sample_' + name)
    act.tags = [TAG]
    act.set_folder_path('KitSamples')
    out.append({'mesh': name, 'kit': kit, 'x': round(x), 'w': round(w), 'h': round(b.max.z - b.min.z)})
    x += w * 0.5 + 150.0
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/PerfTest')
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'kit_samples.json'), 'w'), indent=1)
