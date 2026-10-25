"""Lays out the Megaplant trees in Main beside the nature packs, for comparing them.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/place_megaplants.py"
Re-running removes the actors it placed before (tag MegaplantCompare). The map is not saved.
Big trees (Dead Pine, Aspen) in one row, the saplings (Norway Maple, Rowan, Baltic Pine) in a second, each tree's
bounding box on the ground and side by side with a gap. Outliner folder Packs/Megaplants/<pack>.
Their branches and leaves are Nanite Assembly parts: they show only with r.Nanite.AllowAssemblies (project setting,
read at start) on. Report: Saved/place_megaplants.json.
"""
import json
import os

import unreal

ROOT = '/Game/Megaplant_Library/'
ROWS = [
    # (y of the row's near edge, [(pack label, mesh path without variant letter, variants)])
    (-1150.0, [('DeadPine', 'Tree_Dead_Pine/Tree_Dead_Pine_01/Tree_Dead_Pine_', 'ABCDE'),
               ('Aspen', 'Tree_European_Aspen/Tree_European_Aspen_01/Tree_European_Aspen_01_', 'ABCD')]),
    (250.0, [('NorwayMaple', 'Tree_Norway_Maple/Tree_Norway_Maple_Saplings_01/Tree_Norway_Maple_Sapling_01_', 'ABCD'),
             ('Rowan', 'Tree_Rowan/Tree_Rowan_Saplings_01/Tree_Rowan_Sapling_01_', 'ABCD'),
             ('BalticPineSapling', 'Tree_Baltic_Pine/Tree_Baltic_Pine_Saplings_01/Tree_Baltic_Pine_Sapling_01_', 'ABCD')]),
]
X_START, X_END = 1500.0, 5900.0
GAP = 150.0
GROUND_Z = -129.0
TAG = 'MegaplantCompare'
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'place_megaplants.json')


def main():
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    removed = 0
    for actor in actors.get_all_level_actors():
        if TAG in [str(t) for t in actor.tags]:
            actors.destroy_actor(actor)
            removed += 1
    placed = []
    for y, packs in ROWS:
        x = X_START
        for pack, path, variants in packs:
            for variant in variants:
                asset = ROOT + path + variant
                mesh = unreal.load_asset(asset)
                bounds = mesh.get_bounds()
                lo = bounds.origin - bounds.box_extent
                hi = bounds.origin + bounds.box_extent
                location = unreal.Vector(x - lo.x, y - lo.y, GROUND_Z - lo.z)
                actor = actors.spawn_actor_from_object(mesh, location, unreal.Rotator(0.0, 0.0, 0.0))
                label = 'MP_{}_{}'.format(pack, variant)
                actor.set_actor_label(label)
                actor.set_folder_path('Packs/Megaplants/' + pack)
                actor.tags = [TAG]
                placed.append({'label': label, 'size': [round(hi.x - lo.x), round(hi.y - lo.y), round(hi.z - lo.z)],
                               'x': [round(x), round(x + hi.x - lo.x)], 'y': [round(y), round(y + hi.y - lo.y)],
                               'fits': x + hi.x - lo.x <= X_END})
                x += hi.x - lo.x + GAP
    report = {'removed_before': removed, 'placed': placed,
              'assemblies_on': unreal.SystemLibrary.get_console_variable_int_value('r.Nanite.AllowAssemblies')}
    with open(OUT, 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[PlaceMegaplants] placed {} -> {}'.format(len(placed), OUT))


main()
