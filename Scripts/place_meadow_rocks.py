"""Places medium and large Megascans rocks in the PerfTest meadow: random yaw, a small tilt, sunk 10-25% of their height
into the ground, kept off the meadow trees (tag MeadowTree, 3 m + tree radius) and the player start, at least 4 m apart.
StaticMeshActors in Outliner folder MeadowRocks, tag "MeadowRock" (+ role tag); earlier ones are replaced. Saves the map.
py ".../place_meadow_rocks.py" [large=6] [medium=10] [size_m=60] [seed=11] -> Saved/meadow_rocks.json"""
import json, math, os, random, sys
import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))
LARGE = int(sys.argv[1]) if len(sys.argv) > 1 else 6
MEDIUM = int(sys.argv[2]) if len(sys.argv) > 2 else 10
SIZE = float(sys.argv[3]) * 100 if len(sys.argv) > 3 else 6000.0
random.seed(int(sys.argv[4]) if len(sys.argv) > 4 else 11)
M = '/Game/Fab/Megascans/3D/'
def first_mesh(folder):
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    for d in reg.get_assets_by_path(M + folder, recursive=True):
        if str(d.asset_class_path.asset_name) == 'StaticMesh':
            return unreal.load_asset(str(d.package_name))
LARGE_SET = ['Mossy_Forest_Boulder_wfstcdnaw', 'Beach_Boulder_ukopeess', 'Beach_Boulder_ukopeijs', 'Nordic_Beach_Rocks_ulznddxva', 'Rock_shopk']
MEDIUM_SET = ['Sandstone_Boulder_vmjjfdm', 'Nordic_Beach_Rock_vbumba2fa', 'Beach_Boulder_wenobdn']
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
blocked = [(-1800.0, -1800.0, 600.0)]
for a in actors.get_all_level_actors():
    tags = [str(t) for t in a.tags]
    if 'MeadowRock' in tags:
        actors.destroy_actor(a); continue
    if 'MeadowTree' in tags:
        l = a.get_actor_location(); blocked.append((l.x, l.y, 300.0 + 150.0))
placed, pts = [], []
def spot(r):
    for _ in range(5000):
        x, y = random.uniform(-SIZE / 2 + 300, SIZE / 2 - 300), random.uniform(-SIZE / 2 + 300, SIZE / 2 - 300)
        if all(math.hypot(x - bx, y - by) > br + r for bx, by, br in blocked) and all(math.hypot(x - px, y - py) > 400 + r for px, py in pts):
            pts.append((x, y)); return x, y
for group, names, n, role in (('large', LARGE_SET, LARGE, 'Statik'), ('medium', MEDIUM_SET, MEDIUM, 'Dinamik-toplanir')):
    meshes = [first_mesh(f) for f in names]
    meshes = [m for m in meshes if m]
    for i in range(n):
        mesh = meshes[i % len(meshes)]
        b = mesh.get_bounding_box(); r = max(b.max.x - b.min.x, b.max.y - b.min.y) / 2
        xy = spot(r)
        if not xy:
            continue
        s = random.uniform(0.8, 1.3) if group == 'large' else random.uniform(0.7, 1.2)
        rot = unreal.Rotator(roll=random.uniform(-6, 6), pitch=random.uniform(-6, 6), yaw=random.uniform(0, 360))
        act = actors.spawn_actor_from_object(mesh, unreal.Vector(xy[0], xy[1], 0), rot)
        act.set_actor_scale3d(unreal.Vector(s, s, s))
        o, e = act.get_actor_bounds(False)
        loc = act.get_actor_location(); loc.z += -(o.z - e.z) - 2 * e.z * random.uniform(0.10, 0.25)
        act.set_actor_location(loc, False, False)
        act.tags = [unreal.Name('MeadowRock'), unreal.Name(role)]
        act.set_folder_path('MeadowRocks'); act.set_actor_label('MeadowRock_{}_{:02d}_{}'.format(group, i, mesh.get_name()))
        placed.append([group, mesh.get_name(), round(xy[0] / 100, 1), round(xy[1] / 100, 1), round(s, 2)])
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/PerfTest')
json.dump({'count': len(placed), 'rocks': placed}, open(os.path.join(unreal.Paths.project_saved_dir(), 'meadow_rocks.json'), 'w'), indent=0)
