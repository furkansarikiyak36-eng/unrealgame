"""Lays out every static mesh of the three nature packs in Main, one of each, sorted by pack and sub-folder.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/place_nature_packs.py"
Re-running removes the actors it placed before (tag PackShowcase). The map is not saved.
Each mesh stands on the ground (its bounding box bottom on the ground top), shelf-packed in rows inside free areas;
outliner folder Packs/<pack>/<sub-folder>. Report: Saved/place_nature_packs.json.
"""
import json
import os

import unreal

PACKS = {
    'ProceduralNaturePack': '/Game/ProceduralNaturePack',
    'NatureVol2': '/Game/ProceduralNtr_vol2',
    'OpenWorldFoliage': '/Game/PN_OpenWorldFoliage',
}
# Not props to lay out: starter content, the mannequin, kilometre-wide backdrop mountains, water planes, road splines.
SKIP = ('/EPIC_StarterContent', '/UE4_ThirdPerson', '/background_mountains', '/Meshes/water', '/Spline_Meshes')
# Sky domes and the plain shapes the foliage pack shows its colour examples on.
SKIP_NAMES = ('Skybox', 'cube_3x3_sm', 'cylinder_3x4_sm', 'plane_10x10', 'sphere', 'sphere_UVwrap')
TAG = 'PackShowcase'
GROUND_Z = -129.0
GAP = 100.0
# Free ground in Main (x min, x max, y min, y max), filled in order.
AREAS = [(700.0, 5900.0, 1300.0, 5900.0), (-5900.0, -600.0, 1300.0, 5900.0),
         (1500.0, 5900.0, -1200.0, 1000.0), (-5900.0, -1500.0, 0.0, 1000.0)]
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'place_nature_packs.json')


def main():
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    removed = 0
    for actor in actors.get_all_level_actors():
        if TAG in [str(t) for t in actor.tags]:
            actors.destroy_actor(actor)
            removed += 1

    items = []
    skipped = []
    for pack, root in PACKS.items():
        for data in registry.get_assets_by_path(root, recursive=True):
            if str(data.asset_class_path.asset_name) != 'StaticMesh':
                continue
            path = str(data.package_name)
            if any(s in path for s in SKIP) or path.rsplit('/', 1)[-1] in SKIP_NAMES:
                skipped.append(path)
                continue
            sub = str(data.package_path)[len(root):].strip('/')
            items.append((pack, sub, path))
    items.sort()

    area = 0
    x0, x1, y0, y1 = AREAS[area]
    cx, cy, row_depth = x0, y0, 0.0
    last_pack = None
    placed, overflow, too_big = [], [], []
    for pack, sub, path in items:
        mesh = unreal.load_asset(path)
        box = mesh.get_bounding_box()
        lo, hi = box.min, box.max
        sx, sy = hi.x - lo.x, hi.y - lo.y
        # Larger than any free area: left out, or it would push everything after it out of the areas.
        if not any(sx <= a[1] - a[0] and sy <= a[3] - a[2] for a in AREAS[area:]):
            too_big.append([path, round(sx), round(sy)])
            continue
        # A new pack starts a new row.
        if pack != last_pack and cx > x0:
            cx, cy, row_depth = x0, cy + row_depth + GAP * 2, 0.0
        last_pack = pack
        if cx + sx > x1 and cx > x0:
            cx, cy, row_depth = x0, cy + row_depth + GAP, 0.0
        while cy + sy > y1 or sx > x1 - x0:
            area += 1
            if area >= len(AREAS):
                break
            x0, x1, y0, y1 = AREAS[area]
            cx, cy, row_depth = x0, y0, 0.0
        if area >= len(AREAS):
            overflow.append(path)
            continue
        location = unreal.Vector(cx - lo.x, cy - lo.y, GROUND_Z - lo.z)
        actor = actors.spawn_actor_from_object(mesh, location, unreal.Rotator(0.0, 0.0, 0.0))
        name = path.rsplit('/', 1)[-1]
        actor.set_actor_label('{}_{}'.format(pack, name))
        actor.set_folder_path('Packs/{}/{}'.format(pack, sub.replace('pnp_v2_assets/Assets/', '')))
        actor.tags = [TAG]
        placed.append({'label': '{}_{}'.format(pack, name), 'size': [round(sx), round(sy), round(hi.z - lo.z)],
                       'at': [round(location.x), round(location.y)]})
        cx += sx + GAP
        row_depth = max(row_depth, sy)

    report = {'removed_before': removed, 'placed': len(placed), 'overflow': overflow, 'too_big': too_big, 'skipped': skipped, 'items': placed}
    with open(OUT, 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[PlacePacks] placed {} (overflow {}) -> {}'.format(len(placed), len(overflow), OUT))


main()
