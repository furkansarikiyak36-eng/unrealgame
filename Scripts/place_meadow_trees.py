"""Places KiteDemo trees in the PerfTest meadow (the SIZE_M square at the origin): COUNT trees, at least SPACING_M apart,
random yaw and scale 0.85-1.15, kept off the player start. StaticMeshActors in Outliner folder MeadowTrees, tag
"MeadowTree"; earlier ones are replaced. Saves the map.
py ".../place_meadow_trees.py" [count=20] [spacing_m=8] [size_m=60] [seed=7] -> Saved/meadow_trees.json"""
import json, math, os, random, sys
import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))
COUNT = int(sys.argv[1]) if len(sys.argv) > 1 else 20
SPACING = float(sys.argv[2]) * 100 if len(sys.argv) > 2 else 800.0
SIZE = float(sys.argv[3]) * 100 if len(sys.argv) > 3 else 6000.0
random.seed(int(sys.argv[4]) if len(sys.argv) > 4 else 7)
T = '/Game/KiteDemo/Environments/Trees/'
TREES = [(T + 'ScotsPine_01/ScotsPine_01', 3), (T + 'ScotsPineTall_01/ScotsPineTall_01', 3),
         (T + 'HillTree_02/HillTree_02', 2), (T + 'HillTree_Tall_02/HillTree_Tall_02', 2)]
AVOID = [(-1800.0, -1800.0, 500.0)]          # player start
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in actors.get_all_level_actors():
    if 'MeadowTree' in [str(t) for t in a.tags]:
        actors.destroy_actor(a)
meshes = [(unreal.load_asset(p), w) for p, w in TREES]
pool = [m for m, w in meshes for _ in range(w)]
pts, tries = [], 0
while len(pts) < COUNT and tries < 20000:
    tries += 1
    x, y = random.uniform(-SIZE / 2 + 300, SIZE / 2 - 300), random.uniform(-SIZE / 2 + 300, SIZE / 2 - 300)
    if any(math.hypot(x - px, y - py) < SPACING for px, py in pts) or any(math.hypot(x - ax, y - ay) < r for ax, ay, r in AVOID):
        continue
    pts.append((x, y))
placed = []
for i, (x, y) in enumerate(pts):
    mesh = random.choice(pool)
    act = actors.spawn_actor_from_object(mesh, unreal.Vector(x, y, 0), unreal.Rotator(roll=0.0, pitch=0.0, yaw=random.uniform(0, 360)))
    s = random.uniform(0.85, 1.15); act.set_actor_scale3d(unreal.Vector(s, s, s))
    o, e = act.get_actor_bounds(False)
    act.tags = [unreal.Name('MeadowTree'), unreal.Name('Dinamik-kirilir')]
    act.set_folder_path('MeadowTrees'); act.set_actor_label('MeadowTree_{:02d}_{}'.format(i, mesh.get_name()))
    placed.append([mesh.get_name(), round(x / 100, 1), round(y / 100, 1), round(s, 2)])
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/PerfTest')
json.dump({'count': len(placed), 'trees': placed}, open(os.path.join(unreal.Paths.project_saved_dir(), 'meadow_trees.json'), 'w'), indent=0)
