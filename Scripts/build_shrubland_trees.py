"""Megaplant trees on top of the Shrubland layer in /Game/Maps/PerfTest.

Run in the open editor's Output Log (Cmd):
    py "C:/Users/merts/Downloads/MyProject2/Scripts/build_shrubland_trees.py" <trees> [half_side_m] [max_clump]
Places <trees> Megaplant tree actors (the < 5 M triangle variants kept in /Game/Megaplant_Library) in clumps of
1-max_clump (default 4), up to 10 m apart, over a square of 2 * half_side_m around the origin (default 500 m = 1 km2). Megaplant trees are skeletal meshes built from Nanite assemblies, so each one
is its own actor. No collision; shadows on; kept out of the distance fields like the other plants.
Replaces the previous tree layer (tag ShrublandTrees) and saves the map. Report: Saved/shrubland_trees.json.
"""
import collections
import json
import math
import os
import random
import sys
import time

import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))

CORE = 50000.0
TAG = 'ShrublandTrees'
PREFIXES = ['Tree_Baltic_Pine_Sapling_01_', 'Tree_Dead_Pine_', 'Tree_European_Aspen_01_', 'Tree_Japanese_Cypress_01_',
            'Tree_Norway_Maple_Sapling_01_', 'Tree_Rowan_Sapling_01_']

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
registry = unreal.AssetRegistryHelpers.get_asset_registry()


def main():
    count = int(sys.argv[1]) if len(sys.argv) > 1 else 500
    core = (float(sys.argv[2]) * 100.0) if len(sys.argv) > 2 else CORE
    max_clump = int(sys.argv[3]) if len(sys.argv) > 3 else 4
    for a in actors.get_all_level_actors():
        if TAG in [str(t) for t in a.tags]:
            actors.destroy_actor(a)
    trees = []
    for d in registry.get_assets_by_path('/Game/Megaplant_Library', recursive=True):
        name = str(d.asset_name)
        if str(d.asset_class_path.asset_name) == 'SkeletalMesh' and any(
                name.startswith(p) and len(name) == len(p) + 1 for p in PREFIXES):
            trees.append(unreal.load_asset(str(d.package_name)))
    rng = random.Random(count)
    t0 = time.time()
    used = collections.Counter()
    placed = 0
    while placed < count:
        cx, cy = rng.uniform(-core, core), rng.uniform(-core, core)
        for _ in range(min(rng.randint(1, max_clump), count - placed)):
            d, a = rng.uniform(0, 1000.0), rng.uniform(0, 2 * math.pi)
            mesh = rng.choice(trees)
            x = max(-core, min(core, cx + d * math.cos(a)))     # clumps stay inside the square
            y = max(-core, min(core, cy + d * math.sin(a)))
            actor = actors.spawn_actor_from_object(mesh, unreal.Vector(x, y, 0.0),
                                                   unreal.Rotator(roll=0.0, pitch=0.0, yaw=rng.uniform(0, 360)))
            actor.tags = [TAG]
            actor.set_folder_path('Shrubland/Trees')
            comp = actor.skeletal_mesh_component
            comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            comp.set_editor_property('affect_distance_field_lighting', False)
            used[mesh.get_name()] += 1
            placed += 1
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/PerfTest')
    report = {'trees': placed, 'variants': len(trees), 'per_variant': dict(used), 'seconds': round(time.time() - t0, 1)}
    with open(os.path.join(unreal.Paths.project_saved_dir(), 'shrubland_trees.json'), 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[ShrublandTrees] {} trees, {} variants'.format(placed, len(trees)))


main()
