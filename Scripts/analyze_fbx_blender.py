"""Measures FBX files with Blender in the background, without touching the Unreal project.

blender -b --factory-startup --python Scripts/analyze_fbx_blender.py -- <folder> <out.json>
Per file and mesh: size in cm (Blender metres x 100), where the pivot sits inside the bounding box (0 = min, 1 = max
per axis), the object's own rotation/scale, vertices, triangles, materials, UV layers, non-manifold edges (0 = closed,
watertight), separate parts, and the FBX unit/axis settings Blender read.
"""
import json
import os
import sys

import bmesh
import bpy
from mathutils import Vector

args = sys.argv[sys.argv.index('--') + 1:]
FOLDER, OUT = args[0], args[1]


def parts(bm):
    seen, count = set(), 0
    for v in bm.verts:
        if v in seen:
            continue
        count += 1
        stack = [v]
        while stack:
            cur = stack.pop()
            if cur in seen:
                continue
            seen.add(cur)
            stack.extend(e.other_vert(cur) for e in cur.link_edges)
    return count


def main():
    report = {}
    for root, _, files in os.walk(FOLDER):
        for name in sorted(files):
            if not name.lower().endswith('.fbx'):
                continue
            path = os.path.join(root, name)
            bpy.ops.wm.read_factory_settings(use_empty=True)
            try:
                bpy.ops.import_scene.fbx(filepath=path)
            except Exception as error:
                report[os.path.relpath(path, FOLDER)] = {'error': str(error)[:200]}
                continue
            meshes = []
            for obj in bpy.context.scene.objects:
                if obj.type != 'MESH':
                    continue
                corners = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
                lo = Vector((min(c.x for c in corners), min(c.y for c in corners), min(c.z for c in corners)))
                hi = Vector((max(c.x for c in corners), max(c.y for c in corners), max(c.z for c in corners)))
                size = hi - lo
                origin = obj.matrix_world.translation
                pivot = [round((origin[i] - lo[i]) / size[i], 2) if size[i] > 1e-6 else None for i in range(3)]
                bm = bmesh.new()
                bm.from_mesh(obj.data)
                tris = sum(len(f.verts) - 2 for f in bm.faces)
                nonmanifold = sum(1 for e in bm.edges if not e.is_manifold)
                meshes.append({
                    'object': obj.name,
                    'size_cm': [round(size.x * 100, 1), round(size.y * 100, 1), round(size.z * 100, 1)],
                    'pivot_in_box': pivot,
                    'rot_deg': [round(a * 57.2958, 1) for a in obj.rotation_euler],
                    'scale': [round(s, 3) for s in obj.scale],
                    'verts': len(bm.verts),
                    'tris': tris,
                    'materials': [m.name for m in obj.data.materials if m],
                    'uv_layers': [uv.name for uv in obj.data.uv_layers],
                    'nonmanifold_edges': nonmanifold,
                    'parts': parts(bm),
                })
                bm.free()
            report[os.path.relpath(path, FOLDER).replace('\\', '/')] = {
                'objects': len(bpy.context.scene.objects), 'meshes': meshes,
                'unit_scale': bpy.context.scene.unit_settings.scale_length,
            }
    with open(OUT, 'w') as f:
        json.dump(report, f, indent=1)


main()
