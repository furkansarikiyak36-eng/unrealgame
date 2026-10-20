"""Lays out every static mesh of the given kits on a new map, grouped by kit / sub-folder / kind, to see what we have.

Run in the open editor's Output Log (Cmd):
    py "C:/Users/merts/Downloads/MyProject2/Scripts/build_kit_showcase.py" [kits, comma separated]
Default kits: KiteDemo,ParagonProps. Creates /Game/Maps/KitShowcase from the default template (sky, sun, fog) with a
large grey ground. Each group is a strip: a text label, then its meshes left to right on the ground (2 m apart),
wrapping after ROW_LEN; strips are stacked along +Y. Kinds come from Saved/asset_cost_per_mesh.csv.
Report: Saved/kit_showcase.json (group, count, strip origin).
Append mode (2nd argument "append"): the open KitShowcase map is kept and the new groups are added after its last row;
the ground is stretched to cover them. Skeletal meshes are placed too. Every mesh's real bottom is set 2 cm above the
ground (no z-fighting flicker, no floating pieces with wrong asset bounds).
"""
import collections
import csv
import json
import os
import sys

import unreal

ROW_LEN = 15000.0      # 150 m per row
GAP = 200.0
MAP = '/Game/Maps/KitShowcase'
kits = sys.argv[1].split(',') if len(sys.argv) > 1 else ['KiteDemo', 'ParagonProps']
APPEND = len(sys.argv) > 2 and sys.argv[2] == 'append'
MESH_CLASSES = ('StaticMesh', 'SkeletalMesh') if APPEND else ('StaticMesh',)
SAVED = unreal.Paths.project_saved_dir()
cat = {}
try:
    for r in csv.DictReader(open(os.path.join(SAVED, 'asset_cost_per_mesh.csv'), encoding='utf-8-sig')):
        cat[r['package']] = r['category']
except FileNotFoundError:
    pass

reg = unreal.AssetRegistryHelpers.get_asset_registry()
groups = collections.OrderedDict()
for kit in kits:
    items = []
    for d in reg.get_assets_by_path('/Game/' + kit, recursive=True):
        if str(d.asset_class_path.asset_name) in MESH_CLASSES:
            pkg = str(d.package_name)
            parts = pkg.split('/')
            if kit == 'ParagonProps':
                sub = parts[3]
            elif kit == 'Fab' and len(parts) > 5:
                sub = parts[4]
            elif kit == 'SoulCave' and len(parts) > 4:
                sub = parts[-2]
            else:
                sub = ''
            if str(d.asset_class_path.asset_name) == 'SkeletalMesh':
                sub = (sub + ' skeletal').strip()
            key = ' / '.join(x for x in (kit, sub, cat.get(pkg, 'Diğer')) if x)
            items.append((key, pkg))
    for key, pkg in sorted(items):
        groups.setdefault(key, []).append(pkg)

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ground = None
if APPEND:
    # re-running for the same kits replaces their earlier (possibly partial) groups
    for a in actors.get_all_level_actors():
        if not isinstance(a, (unreal.StaticMeshActor, unreal.SkeletalMeshActor, unreal.TextRenderActor)) or a.get_actor_label() == 'ShowcaseGround':
            continue  # never the template lights (their folder is also called "Lighting")
        folder = str(a.get_folder_path())
        if any(folder == k or folder.startswith(k + '/') or a.get_actor_label().startswith('Label_' + k) for k in kits):
            actors.destroy_actor(a)
    y = 0.0
    for a in actors.get_all_level_actors():
        if a.get_actor_label() == 'ShowcaseGround':
            ground = a
        elif isinstance(a, (unreal.StaticMeshActor, unreal.SkeletalMeshActor)):
            o, e = a.get_actor_bounds(False)
            y = max(y, o.y + e.y)
    y += GAP * 6
    try:
        report = json.load(open(os.path.join(SAVED, 'kit_showcase.json'), encoding='utf-8'))['groups']
    except Exception:
        report = []
else:
    unreal.EditorLoadingAndSavingUtils.new_map_from_template('/Engine/Maps/Templates/Template_Default', False)
    for a in actors.get_all_level_actors():
        if isinstance(a, unreal.StaticMeshActor):
            actors.destroy_actor(a)
    report = []
    y = 0.0


def mesh_box(mesh):
    if isinstance(mesh, unreal.StaticMesh):
        b = mesh.get_bounding_box()
        return b.min, b.max
    bb = mesh.get_bounds()
    o, e = bb.origin, bb.box_extent
    return unreal.Vector(o.x - e.x, o.y - e.y, o.z - e.z), unreal.Vector(o.x + e.x, o.y + e.y, o.z + e.z)


placed = 0
for key, pkgs in groups.items():
    label = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(-1500.0, y, 300.0),
                                          unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0))
    label.text_render.set_editor_property('text', '{} ({})'.format(key, len(pkgs)))
    label.text_render.set_editor_property('world_size', 250.0)
    label.text_render.set_editor_property('horizontal_alignment', unreal.HorizTextAligment.EHTA_RIGHT)
    label.set_actor_label('Label_' + key.replace(' / ', '_'))
    label.set_folder_path('Labels')
    x, row_depth, strip_start = 0.0, 0.0, y
    for pkg in pkgs:
        mesh = unreal.load_asset(pkg)
        if not mesh:
            continue
        bmin, bmax = mesh_box(mesh)
        w, dpt = min(bmax.x - bmin.x, ROW_LEN), min(bmax.y - bmin.y, ROW_LEN)
        if x > 0 and x + w > ROW_LEN:
            x = 0.0
            y += row_depth + GAP
            row_depth = 0.0
        loc = unreal.Vector(x - bmin.x, y - bmin.y, -bmin.z)
        act = actors.spawn_actor_from_object(mesh, loc, unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
        if not act:
            continue
        o, e = act.get_actor_bounds(False)
        loc.z += 2.0 - (o.z - e.z)
        act.set_actor_location(loc, False, False)
        placed += 1
        act.set_actor_label(mesh.get_name())
        act.set_folder_path(key.replace(' / ', '/'))
        if placed % 25 == 0:
            mesh = None
            unreal.SystemLibrary.collect_garbage()
        x += w + GAP
        row_depth = max(row_depth, dpt)
    report.append({'group': key, 'count': len(pkgs), 'y_start_m': round(strip_start / 100), 'y_end_m': round((y + row_depth) / 100)})
    y += row_depth + GAP * 6

span_y = y
if ground is None:
    ground = actors.spawn_actor_from_object(unreal.load_asset('/Engine/BasicShapes/Cube'), unreal.Vector(ROW_LEN / 2, span_y / 2, -50.0))
ground.set_actor_location(unreal.Vector(ROW_LEN / 2, span_y / 2, -50.0), False, False)
ground.set_actor_scale3d(unreal.Vector((ROW_LEN + 6000) / 100.0, (span_y + 6000) / 100.0, 1.0))
ground.set_actor_label('ShowcaseGround')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)
json.dump({'map': MAP, 'groups': report, 'size_m': [round(ROW_LEN / 100), round(span_y / 100)]},
          open(os.path.join(SAVED, 'kit_showcase.json'), 'w'), indent=1, ensure_ascii=False)
unreal.log('[KitShowcase] {} groups'.format(len(report)))
