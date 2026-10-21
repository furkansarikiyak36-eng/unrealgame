"""Lays out in Main, on new ground east of the old one, a tidy showcase of the added content:
the light Megaplant trees and plants, the new Fab Megascans plants and 3D models, the new Fab surfaces (each on a
2 x 2 m sample tile) and the UnrealExport logs.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/place_showcase.py"
Re-running removes what it placed before (tags Showcase, ShowcaseGround) and the older LightSet layout.
Layout: one row per pack, the row's meshes side by side with an even gap and their centres on one line, a sign with
the pack name at the start of the row. Rows stack in columns 54 m wide; a section starts a new row. The ground is
copies of the level's Ground cube (same material and height), one per 120 x 120 m tile the columns reach.
Outliner: Showcase/<section>/<pack>. The map is not saved. Report: Saved/place_showcase.json.
"""
import json
import os

import unreal

TAGS_OLD = ('Showcase', 'ShowcaseGround', 'LightSet')
TAG, TAG_GROUND = 'Showcase', 'ShowcaseGround'
GROUND_TOP = -129.0
TILE = 12000.0
FIRST_TILE_X = 12000.0          # tile centres: 12000, 24000, ... (the old ground spans -6000..6000)
COLUMN_W, MARGIN = 5400.0, 300.0
GAP_X, GAP_ROW, GAP_SECTION = 80.0, 180.0, 500.0
Y_START, Y_END = -5700.0, 5700.0
SIGN_SIZE = 28.0

MEGA_TREES = {'Tree_Norway_Maple_Sapling_01_': 'ABCD', 'Tree_Japanese_Cypress_01_': 'CEFG',
              'Tree_Rowan_Sapling_01_': 'ABCD', 'Tree_Baltic_Pine_Sapling_01_': 'ABCD', 'Tree_Dead_Pine_': 'ABCDE',
              'Tree_European_Aspen_01_': 'D'}
# Plants come from Megascans only (decided 2026-10-19); the Megaplant plants and shrubs are left out.
MEGA_PLANTS = {}
FAB_3D_NEW = ['Military_Trenches_Equipment_Stick_Wood_S_01_xflicih', 'Military_Trenches_Equipment_Stick_Wood_S_04_xfekfgv',
              'Puffball_Mushrooms_qdzs3', 'Bolete_Mushrooms_qdzrT', 'Bolete_Mushrooms_pdvcB']
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'place_showcase.json')

registry = unreal.AssetRegistryHelpers.get_asset_registry()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def assets_under(path, cls):
    return sorted((d for d in registry.get_assets_by_path(path, recursive=True)
                   if str(d.asset_class_path.asset_name) == cls), key=lambda d: str(d.asset_name))


def bounds(asset):
    if isinstance(asset, unreal.SkeletalMesh):
        b = asset.get_bounds()
        return b.origin - b.box_extent, b.origin + b.box_extent
    box = asset.get_bounding_box()
    return box.min, box.max


def collect():
    """[(section, pack, [(label, asset, override_material)])] in display order."""
    sections = []
    mega = {}
    for d in assets_under('/Game/Megaplant_Library', 'SkeletalMesh'):
        name = str(d.asset_name)
        for prefix, variants in list(MEGA_TREES.items()) + list(MEGA_PLANTS.items()):
            if name.startswith(prefix) and name[len(prefix):] in variants:
                kind = 'MegaplantTrees' if prefix in MEGA_TREES else 'MegaplantPlants'
                mega.setdefault((kind, prefix.rstrip('_')), []).append((name, unreal.load_asset(str(d.package_name)), None))
    for kind in ('MegaplantTrees', 'MegaplantPlants'):
        for (k, pack), items in sorted(mega.items()):
            if k == kind:
                sections.append((kind, pack, items))
    plants_root = '/Game/Fab/Megascans/Plants'
    for sub in sorted(str(p) for p in registry.get_sub_paths(plants_root, False)):
        items = [(str(d.asset_name), unreal.load_asset(str(d.package_name)), None) for d in assets_under(sub, 'StaticMesh')]
        if items:
            sections.append(('FabPlants', sub.rsplit('/', 1)[-1], items))
    for folder in FAB_3D_NEW:
        items = [(str(d.asset_name), unreal.load_asset(str(d.package_name)), None)
                 for d in assets_under('/Game/Fab/Megascans/3D/' + folder, 'StaticMesh')]
        if items:
            sections.append(('Fab3D', folder, items))
    plane = unreal.load_asset('/Engine/BasicShapes/Plane')
    surfaces = []
    for sub in sorted(str(p) for p in registry.get_sub_paths('/Game/Fab/Megascans/Surfaces', False)):
        mis = assets_under(sub, 'MaterialInstanceConstant')
        if mis:
            surfaces.append((sub.rsplit('/', 1)[-1], plane, unreal.load_asset(str(mis[0].package_name))))
    for i in range(0, len(surfaces), 11):
        sections.append(('Surfaces', 'Surfaces_{}'.format(i // 11 + 1), surfaces[i:i + 11]))
    logs = {}
    for d in assets_under('/Game/UnrealExport', 'StaticMesh'):
        mesh = unreal.load_asset(str(d.package_name))
        lo, hi = bounds(mesh)
        group = 'Archive' if '/Archive' in str(d.package_path) else 'Length_{}'.format(int(round(hi.x - lo.x)))
        logs.setdefault(group, []).append((str(d.asset_name), mesh, None))
    for group in sorted(logs, key=lambda g: (g == 'Archive', int(g.split('_')[1]) if g != 'Archive' else 0)):
        sections.append(('Logs', group, logs[group]))
    return sections


def spawn(asset, location, label, folder, material=None, scale=None):
    actor = actors.spawn_actor_from_object(asset, location, unreal.Rotator(0.0, 0.0, 0.0))
    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    actor.tags = [TAG]
    if scale:
        actor.set_actor_scale3d(scale)
    if material:
        actor.static_mesh_component.set_material(0, material)
    return actor


def sign(text, x, y, folder):
    actor = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(x, y, GROUND_TOP + 5.0),
                                          unreal.Rotator(0.0, -90.0, 0.0))
    comp = actor.text_render
    comp.set_text(text)
    comp.set_world_size(SIGN_SIZE)
    comp.set_editor_property('horizontal_alignment', unreal.HorizTextAligment.EHTA_LEFT)
    actor.set_actor_label('Sign_' + text)
    actor.set_folder_path(folder)
    actor.tags = [TAG]


def main():
    removed = 0
    for actor in actors.get_all_level_actors():
        if any(t in [str(x) for x in actor.tags] for t in TAGS_OLD):
            actors.destroy_actor(actor)
            removed += 1

    column = 0
    col_x0 = lambda c: FIRST_TILE_X - TILE / 2 + MARGIN + c * (COLUMN_W + 2 * MARGIN)
    y = Y_START
    last_section = None
    report = {'rows': [], 'items': []}
    for section, pack, items in collect():
        sizes = []
        for label, asset, material in items:
            lo, hi = bounds(asset)
            if material:  # 100 x 100 plane scaled to a 2 x 2 m tile
                lo, hi = unreal.Vector(-100, -100, 0), unreal.Vector(100, 100, 0)
            sizes.append((lo, hi))
        # Wrap a pack wider than a column onto more lines.
        lines, line, width = [], [], 0.0
        for item, (lo, hi) in zip(items, sizes):
            w = hi.x - lo.x
            if line and width + GAP_X + w > COLUMN_W:
                lines.append(line)
                line, width = [], 0.0
            width += (GAP_X if line else 0.0) + w
            line.append((item, lo, hi))
        if line:
            lines.append(line)
        if last_section is not None and section != last_section:
            y += GAP_SECTION
        last_section = section
        for n, line in enumerate(lines):
            depth = max(hi.y - lo.y for _, lo, hi in line)
            if y + depth > Y_END:
                column += 1
                y = Y_START
            x = col_x0(column)
            centre_y = y + depth / 2.0
            if n == 0:
                sign('{} / {}'.format(section, pack), x, y - 60.0, 'Showcase/{}/{}'.format(section, pack))
            for (label, asset, material), lo, hi in line:
                w = hi.x - lo.x
                mid_y = (lo.y + hi.y) / 2.0
                if material:
                    spawn(asset, unreal.Vector(x + w / 2.0, centre_y, GROUND_TOP + 0.5),
                          label, 'Showcase/{}/{}'.format(section, pack), material, unreal.Vector(2, 2, 1))
                else:
                    spawn(asset, unreal.Vector(x - lo.x, centre_y - mid_y, GROUND_TOP - lo.z),
                          label, 'Showcase/{}/{}'.format(section, pack))
                report['items'].append({'section': section, 'pack': pack, 'label': label,
                                        'size': [round(w), round(hi.y - lo.y), round(hi.z - lo.z)],
                                        'x': round(x + w / 2.0), 'y': round(centre_y)})
                x += w + GAP_X
            report['rows'].append({'section': section, 'pack': pack, 'line': n, 'column': column,
                                   'y': [round(y), round(y + depth)], 'count': len(line)})
            y += depth + GAP_ROW

    # Ground: a copy of the level's Ground per tile the columns reach.
    ground = next(a for a in actors.get_all_level_actors() if a.get_actor_label() == 'Ground')
    tiles = int((col_x0(column) + COLUMN_W - (FIRST_TILE_X - TILE / 2)) // TILE) + 1
    for t in range(tiles):
        copy = actors.duplicate_actor(ground)
        loc = ground.get_actor_location()
        copy.set_actor_location(unreal.Vector(FIRST_TILE_X + t * TILE, loc.y, loc.z), False, False)
        copy.set_actor_label('Ground_Showcase_{}'.format(t + 1))
        copy.set_folder_path('Showcase')
        copy.tags = [TAG_GROUND]
    report.update({'removed_before': removed, 'columns': column + 1, 'tiles': tiles, 'placed': len(report['items'])})
    with open(OUT, 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[PlaceShowcase] {} items, {} columns, {} tiles -> {}'.format(len(report['items']), column + 1, tiles, OUT))


main()
