"""Lays out in Main the light Megaplant variants (trees under 5 M Nanite triangles, plants under 1 M) and the
UnrealExport logs, replacing the earlier Megaplant comparison rows.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/place_light_set.py"
Removes the actors tagged MegaplantCompare (place_megaplants.py) and LightSet (an earlier run of this). Each mesh's
bounding box sits on the ground, shelf-packed in rows with a gap; trees first then plants in the strip south of the
nature packs, the logs (sorted by length) in the free strip on the west. Outliner: LightSet/Trees/<pack>,
LightSet/Plants/<pack>, LightSet/Logs. The map is not saved. Report: Saved/place_light_set.json.
"""
import json
import os

import unreal

TREES = {
    'Tree_Norway_Maple_Sapling_01_': 'ABCD', 'Tree_Japanese_Cypress_01_': 'CEFG', 'Tree_Rowan_Sapling_01_': 'ABCD',
    'Tree_Baltic_Pine_Sapling_01_': 'ABCD', 'Tree_Dead_Pine_': 'ABCDE', 'Tree_European_Aspen_01_': 'D',
}
PLANTS = {
    'Plant_Small_Balsam_01_': 'ABCDE', 'Plant_Common_Bulrush_01_': 'ABCDEF', 'Plant_Lingonberry_01_': 'ABCDEFG',
    'Plant_Wood_Anemone_01_': 'ABCD', 'Plant_Bilberry_01_': 'ABCD', 'Plant_Stinging_Nettle_01_': 'ABCDEF',
    'Plant_Dry_Common_Reed_': 'B', 'Shrub_Huckleberry_Oak_01_': 'D',
}
TAG = 'LightSet'
OLD_TAGS = ('MegaplantCompare', TAG)
GROUND_Z = -129.0
GAP = 120.0
# Trees and plants: first free spot on the ground north of the props and rocks (x min, x max, y min, y max).
GREEN_REGION = (-5900.0, 5900.0, -1300.0, 5900.0)
LOG_AREAS = [(-5900.0, -1500.0, 0.0, 1000.0)]
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'place_light_set.json')


def wanted(names):
    return {prefix + v for prefix, variants in names.items() for v in variants}


def bounds(asset):
    if isinstance(asset, unreal.SkeletalMesh):
        b = asset.get_bounds()
        return b.origin - b.box_extent, b.origin + b.box_extent
    box = asset.get_bounding_box()
    return box.min, box.max


def pack_rows(items, areas, actors, folder_of, report, overflow):
    area = 0
    x0, x1, y0, y1 = areas[area]
    cx, cy, depth, last = x0, y0, 0.0, None
    for group, label, asset in items:
        lo, hi = bounds(asset)
        sx, sy = hi.x - lo.x, hi.y - lo.y
        if group != last and cx > x0:
            cx, cy, depth = x0, cy + depth + GAP, 0.0
        last = group
        if cx + sx > x1 and cx > x0:
            cx, cy, depth = x0, cy + depth + GAP, 0.0
        while cy + sy > y1:
            area += 1
            if area >= len(areas):
                break
            x0, x1, y0, y1 = areas[area]
            cx, cy, depth = x0, y0, 0.0
        if area >= len(areas):
            overflow.append(label)
            continue
        location = unreal.Vector(cx - lo.x, cy - lo.y, GROUND_Z - lo.z)
        actor = actors.spawn_actor_from_object(asset, location, unreal.Rotator(0.0, 0.0, 0.0))
        actor.set_actor_label(label)
        actor.set_folder_path(folder_of(group))
        actor.tags = [TAG]
        report.append({'label': label, 'size': [round(sx), round(sy), round(hi.z - lo.z)],
                       'x': [round(cx), round(cx + sx)], 'y': [round(cy), round(cy + sy)]})
        cx += sx + GAP
        depth = max(depth, sy)


def occupied_boxes(actors):
    """2D boxes of what already stands in the level (ground, sky and lights left out)."""
    boxes = []
    for actor in actors.get_all_level_actors():
        label = actor.get_actor_label()
        if label == 'Ground' or not isinstance(actor, (unreal.StaticMeshActor, unreal.SkeletalMeshActor, unreal.TextRenderActor, unreal.PlayerStart)):
            continue
        origin, extent = actor.get_actor_bounds(False)
        if extent.x > 1500 or extent.y > 1500:
            continue
        boxes.append((origin.x - extent.x, origin.x + extent.x, origin.y - extent.y, origin.y + extent.y))
    return boxes


def first_fit(items, region, boxes, actors, folder_of, report, overflow, step=50.0):
    """Each mesh at the first free spot, scanning rows from the region's near edge; a new kind starts a new row."""
    x_min, x_max, y_min, y_max = region
    row_y, last, placed_far = y_min, None, y_min
    for group, label, asset in items:
        lo, hi = bounds(asset)
        sx, sy = hi.x - lo.x, hi.y - lo.y
        if last is not None and group[0] != last[0]:
            # The next kind starts beyond what this call has placed so far, not beyond everything in the level.
            row_y = placed_far + GAP
        last = group
        spot = None
        y = row_y
        while spot is None and y + sy <= y_max:
            x = x_min
            while x + sx <= x_max:
                if not any(x - GAP < b[1] and x + sx + GAP > b[0] and y - GAP < b[3] and y + sy + GAP > b[2] for b in boxes):
                    spot = (x, y)
                    break
                x += step
            y += step
        if spot is None:
            overflow.append(label)
            continue
        x, y = spot
        location = unreal.Vector(x - lo.x, y - lo.y, GROUND_Z - lo.z)
        actor = actors.spawn_actor_from_object(asset, location, unreal.Rotator(0.0, 0.0, 0.0))
        actor.set_actor_label(label)
        actor.set_folder_path(folder_of(group))
        actor.tags = [TAG]
        boxes.append((x, x + sx, y, y + sy))
        placed_far = max(placed_far, y + sy)
        report.append({'label': label, 'size': [round(sx), round(sy), round(hi.z - lo.z)],
                       'x': [round(x), round(x + sx)], 'y': [round(y), round(y + sy)]})


def main():
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    removed = 0
    for actor in actors.get_all_level_actors():
        if any(t in [str(x) for x in actor.tags] for t in OLD_TAGS):
            actors.destroy_actor(actor)
            removed += 1

    trees, plants = wanted(TREES), wanted(PLANTS)
    green = []
    for data in registry.get_assets_by_path('/Game/Megaplant_Library', recursive=True):
        if str(data.asset_class_path.asset_name) != 'SkeletalMesh':
            continue
        name = str(data.asset_name)
        pack = str(data.package_path).split('/')[3]
        if name in trees or name in plants:
            kind = 'Trees' if name in trees else 'Plants'
            green.append(((kind, pack), name, unreal.load_asset(str(data.package_name))))
    green.sort(key=lambda g: (g[0][0] != 'Trees', g[0][1], g[1]))

    logs = []
    for data in registry.get_assets_by_path('/Game/UnrealExport', recursive=True):
        if str(data.asset_class_path.asset_name) == 'StaticMesh':
            mesh = unreal.load_asset(str(data.package_name))
            lo, hi = bounds(mesh)
            logs.append((('Logs', 'Archive' if '/Archive' in str(data.package_path) else ''), str(data.asset_name), mesh, hi.x - lo.x))
    logs.sort(key=lambda g: (g[0][1], g[3], g[1]))

    placed, overflow = [], []
    pack_rows([(g, n, m) for g, n, m, _ in logs], LOG_AREAS, actors,
              lambda g: 'LightSet/Logs' + ('/Archive' if g[1] else ''), placed, overflow)
    first_fit(green, GREEN_REGION, occupied_boxes(actors), actors,
              lambda g: 'LightSet/{}/{}'.format(g[0], g[1]), placed, overflow)
    report = {'removed_before': removed, 'found_green': len(green), 'found_logs': len(logs),
              'placed': len(placed), 'overflow': overflow, 'items': placed}
    with open(OUT, 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[PlaceLightSet] placed {} (overflow {}) -> {}'.format(len(placed), len(overflow), OUT))


main()
