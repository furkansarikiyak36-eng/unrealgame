"""Editor: MechanicsTest = template sky/sun/fog + floor, nothing else; then 50 raw logs (SM_RawLog_600 / 300 / 150 / 75, one
row per length, become carryable CarvableLogs on play) and 50 stones in three rows: 17 stones (Small_Stones_Pack, 50-75 cm, become stone pickups on play), 17 wall stones (120-170 cm), 16 cobblestones (grey 32-40 cm, the smallest; brown 1.5x: 48-60 cm).
Removes every ALogShowcase and earlier fill actors first. -> Saved/mechanics_fill.json"""
import json, os, random
import unreal
PATH = '/Game/Maps/MechanicsTest'
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level(PATH)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_outer().get_path_name() == PATH, 'wrong map open'
res = {'removed': []}
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.LogShowcase) or str(a.get_folder_path()) == 'MechanicsFill':
        res['removed'].append(a.get_actor_label()); eas.destroy_actor(a)
LENGTHS = [600, 300, 150, 75]
COUNTS = [12, 13, 13, 12]
log_meshes = {L: unreal.load_asset('/Game/UnrealExport/SM_RawLog_%d' % L) for L in LENGTHS}
reg = unreal.AssetRegistryHelpers.get_asset_registry()
stones = sorted(str(d.package_name) for d in reg.get_assets_by_path('/Game/Fab/Megascans/3D/Small_Stones_Pack_vcljbb1iw', recursive=True)
                if str(d.asset_class_path.asset_name) == 'StaticMesh' and 'LOD0' in str(d.asset_name))
WALL = ['/Game/Fab/Megascans/3D/Castle_Wall_Stone_sdAta/Low/sdAta_tier_3/StaticMeshes/sdAta_tier_3']
GRAY = ['/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Gray_Pack_xksldamqx/Low/xksldamqx_tier_3/StaticMeshes/xksldamqx_LOD0_TIER3_%03d' % i for i in range(6)]
BROWN = ['/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Brown_Pack_xkqmdhaqx/Low/xkqmdhaqx_tier_3/StaticMeshes/xkqmdhaqx_LOD0_TIER3_%03d' % i for i in range(7)]
res['stone_meshes'] = len(stones)
random.seed(7)
def place(mesh, loc, yaw, label, size=None):
    a = eas.spawn_actor_from_object(mesh, loc, unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    if size:   # largest side in cm
        b = mesh.get_bounding_box()
        sc = size / max(b.max.x - b.min.x, b.max.y - b.min.y, b.max.z - b.min.z)
        a.set_actor_scale3d(unreal.Vector(sc, sc, sc))
    o, e = a.get_actor_bounds(False)
    loc.z += 2.0 - (o.z - e.z)
    a.set_actor_location(loc, False, False)
    a.set_actor_label(label); a.set_folder_path('MechanicsFill')
def place_logs():
    n = 0
    for row, (L, count) in enumerate(zip(LENGTHS, COUNTS)):
        for k in range(count):
            place(log_meshes[L], unreal.Vector(300 + k * (L + 120), -1200 + row * 300, 0), 0.0, 'Log_%02d_%d' % (n, L))
            n += 1
place_logs()
# Stone chain rows: stones (become pickups on play), wall stones (what a medium stone is chiselled into), cobblestones (a building stone chiselled), 17 + 17 + 16.
for i in range(17):
    place(unreal.load_asset(stones[i % len(stones)]), unreal.Vector(300 + i * 450, 600, 0), random.uniform(0, 360), 'Stone_%02d' % i, size=random.uniform(50, 75))
wall = unreal.load_asset(WALL[0])
for i in range(17):
    place(wall, unreal.Vector(300 + i * 450, 900, 0), random.uniform(0, 360), 'WallStone_%02d' % i, size=random.uniform(120, 170))
for i in range(16):
    place(unreal.load_asset((GRAY if i % 2 == 0 else BROWN)[(i // 2) % 6]), unreal.Vector(300 + i * 450, 1200, 0), random.uniform(0, 360), 'Cobble_%02d' % i, size=random.uniform(32, 40) * (1.0 if i % 2 == 0 else 1.5))
res['saved'] = unreal.EditorLoadingAndSavingUtils.save_current_level()
json.dump(res, open(os.path.join(unreal.Paths.project_saved_dir(), 'mechanics_fill.json'), 'w'), indent=1)
