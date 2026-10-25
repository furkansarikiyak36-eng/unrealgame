"""Rebuilds the LogWallTest walls from normalised scanned logs, each course fitted to the one below.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/build_logwall_fit.py"

1. Normalise (new assets under /Game/LogWall/Meshes, the Fab originals are left alone):
   - the spine is straightened (STRAIGHTEN of the bend removed), the log centred on X,
   - the diameter brought to TARGET_D, taper and bumps pulled ROUND of the way to it (bark detail kept),
   - the length snapped to whole 65 cm modules (the game's log grid), both ragged ends sawn off flat and capped
     with cut-wood end grain.
2. Fit: every course tries the log in 8 poses (roll 0/90/180/270, ends swapped or not) and keeps the pose whose
   underside follows the top of the course below most evenly; it is lowered until it sinks at least MIN_OVERLAP into
   that course everywhere along its length, so there is no see-through gap anywhere.
3. Walls: one log per course (no butt joints), on the old wall lines; the oak log is too crooked for walls and is
   left out; a corner on the last line shows two walls locked together half a course apart.
4. Six joint faces on every fitted mesh, as sockets: Snap_Top/Bottom/EndA/EndB/SideL/SideR. Top and Bottom (and the
   two sides) sit half the measured stacking step from the axis, so a log joined face to opposite face sinks at least
   MIN_OVERLAP into the other everywhere; the ends sit at the sawn ends. One example pair per log and face (6 x 3 =
   18), with a marker at the joint, the joint written on both logs as an actor tag and into the report.
5. Beside each wall line a floor of logs lying side by side, each fitted sideways to its neighbour the same way; and away
   from the walls a square of each log, 2 along X and 2 along Y half a course higher, crossing at four corners (the
   crossing logs sink into each other where a saddle notch would be cut).
"""
import json
import math
import os

import unreal

MAP = '/Game/Maps/LogWallTest'
OUT_DIR = '/Game/LogWall/Meshes'
FAB = '/Game/Fab/Megascans/3D'
# Fab folder per log. The best downloaded quality is used (High, then Medium, then Low; the most detailed tier): the
# first download was Low (tier 3 mesh, 1K textures), which looks flat beyond a few metres.
LOGS = {
    'J': 'Old_Wooden_Log_wcjheff',
    'O': 'Old_Wooden_Log_wcokfdy',
    'L': 'Old_Wooden_Log_wcolfdz',
}
QUALITIES = ('High', 'Medium', 'Low')
# Logs whose own scanned end is not end grain (wcokfdy's is hollow/rotten) borrow another log's end picture and
# material for their sawn caps.
END_GRAIN_FROM = {'O': 'J'}
GRAIN = {}


def best_log_mesh(folder):
    """Path of the most detailed downloaded mesh of a Fab log: /Game/Fab/Megascans/3D/<folder>/<Quality>/<id>_tier_<n>/..."""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    for quality in QUALITIES:
        found = [str(a.package_name) for a in registry.get_assets_by_path('{}/{}/{}'.format(FAB, folder, quality), recursive=True)
                 if str(a.asset_class_path.asset_name) == 'StaticMesh']
        if found:
            # tier_0 is the most detailed.
            return sorted(found, key=lambda p: p.rsplit('_tier_', 1)[-1])[0]
    raise RuntimeError('No mesh downloaded for ' + folder)
# Wall line (y) per log; the corner uses the straightest log.
WALL_Y = {'J': 0.0, 'O': 400.0, 'L': 800.0}
CORNER_LOG, CORNER_Y = 'L', 1200.0
# Beside each wall, logs lying side by side on the ground (a floor): how the logs meet sideways.
FLOOR_LOGS = 5
FLOOR_GAP = 80.0
# A block of each log: BLOCK logs end to end, BLOCK side by side, BLOCK courses high; each log fitted to the one
# below it and the one beside it. Blocks lie side by side towards -Y from BLOCK_Y0.
BLOCK = 7
BLOCK_Y0 = -700.0
BLOCK_GAP = 150.0
# The six faces a log joins by, as sockets on the fitted meshes (Snap_<Face>); the opposite face of the other log.
FACES = [('Top', 'Üst'), ('Bottom', 'Alt'), ('EndA', 'Uç A'), ('EndB', 'Uç B'), ('SideL', 'Sol'), ('SideR', 'Sağ')]
OPPOSITE = {'Top': 'Bottom', 'Bottom': 'Top', 'EndA': 'EndB', 'EndB': 'EndA', 'SideL': 'SideR', 'SideR': 'SideL'}
# How far a sawn end reaches into the end it butts against.
END_TOUCH = 0.2
# One example pair per log and face: a row per log towards -Y, the six faces along X.
JOINT_ROWS = {'J': -800.0, 'O': -1400.0, 'L': -2000.0}
JOINT_GAP = 150.0
# Face arrows, as on the Fab logs in Main (Snap_<Face> ArrowComponents, tag SnapLog): editor-only, red, the face a
# joint uses green.
ARROW_LENGTH = 45.0
ARROW_RED = (255, 40, 30)
ARROW_GREEN = (40, 230, 60)
FACE_NAMES = dict(FACES)
# A square of each log away from the walls: 2 logs along X, 2 along Y half a course higher, four locked corners.
CRIB_X = 1000.0
CRIB_Y = {'J': 0.0, 'O': 650.0, 'L': 1350.0}

MODULE = 65.0
TARGET_D = 36.0
STRAIGHTEN = 0.9
ROUND = 0.5
BINS = 24
# The scanned ends are ragged: each end is sawn off flat where the section is whole (see end_trim), the log scaled to
# the module length between the cuts, and the ends capped with cut-wood end grain, so two logs butt face to face.
CUT_MATERIAL = '/Game/Building/Materials/M_CutWood'
CUT_UV_CM = 40.0
WALL_HEIGHT = 240.0
MIN_OVERLAP = 2.0
# How deep the first course sits in the ground, and how far a corner log runs past the crossing.
GROUND_SINK = 4.0
CORNER_RUNOUT = 0.6 * TARGET_D

REPORT = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'build_logwall_fit.json')
LOG = []


def log(*parts):
    text = ' '.join(str(p) for p in parts)
    LOG.append(text)
    unreal.log('[LogWallFit] ' + text)


def smooth(values, passes=2):
    for _ in range(passes):
        values = [(values[max(0, i - 1)] + 2 * values[i] + values[min(len(values) - 1, i + 1)]) / 4.0
                  for i in range(len(values))]
    return values


def vector_list_to_array(vlist):
    for owner in ('GeometryScript_List', 'GeometryScript_ListUtilities'):
        lib = getattr(unreal, owner, None)
        if lib and hasattr(lib, 'convert_vector_list_to_array'):
            return lib.convert_vector_list_to_array(vlist)
    raise RuntimeError('No vector list conversion in this engine version')


def array_to_vector_list(arr):
    for owner in ('GeometryScript_List', 'GeometryScript_ListUtilities'):
        lib = getattr(unreal, owner, None)
        if lib and hasattr(lib, 'convert_array_to_vector_list'):
            return lib.convert_array_to_vector_list(arr)
    raise RuntimeError('No vector list conversion in this engine version')


# ----------------------------------------------------------------------------- normalise

def normalise(key, path):
    source = unreal.load_asset(path)
    if source is None:
        raise RuntimeError('Missing log mesh ' + path)
    mesh = unreal.DynamicMesh()
    lod = unreal.GeometryScriptMeshReadLOD()
    lod.lod_type = unreal.GeometryScriptLODType.SOURCE_MODEL
    result = unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(
        source, mesh, unreal.GeometryScriptCopyMeshFromAssetOptions(), lod)
    mesh = next(r for r in (result if isinstance(result, tuple) else (result,)) if isinstance(r, unreal.DynamicMesh))
    result = unreal.GeometryScript_MeshQueries.get_all_vertex_positions(mesh, False)
    # (mesh, position_list, has_gaps) in this engine version; older ones leave the mesh out.
    positions = next(r for r in (result if isinstance(result, tuple) else (result,)) if isinstance(r, unreal.GeometryScriptVectorList))
    pts = [(p.x, p.y, p.z) for p in vector_list_to_array(positions)]

    lo = min(p[0] for p in pts)
    hi = max(p[0] for p in pts)
    length = hi - lo
    mid = (lo + hi) / 2.0

    def bin_of(x):
        return min(BINS - 1, max(0, int((x - lo) / length * BINS)))

    groups = [[] for _ in range(BINS)]
    for p in pts:
        groups[bin_of(p[0])].append(p)
    cy, cz, rad = [], [], []
    for g in groups:
        if not g:
            cy.append(None); cz.append(None); rad.append(None)
            continue
        ys = [p[1] for p in g]
        zs = [p[2] for p in g]
        y0, z0 = (min(ys) + max(ys)) / 2.0, (min(zs) + max(zs)) / 2.0
        cy.append(y0); cz.append(z0)
        rad.append(sum(math.hypot(p[1] - y0, p[2] - z0) for p in g) / len(g))
    # Fill empty bins from their neighbours, then smooth.
    for arr in (cy, cz, rad):
        known = [i for i, v in enumerate(arr) if v is not None]
        for i in range(BINS):
            if arr[i] is None:
                nearest = min(known, key=lambda k: abs(k - i))
                arr[i] = arr[nearest]
    cy, cz, rad = smooth(cy), smooth(cz), smooth(rad)
    mean_y, mean_z = sum(cy) / BINS, sum(cz) / BINS
    mean_r = sum(rad) / BINS
    target_r = TARGET_D / 2.0
    scale_r = target_r / mean_r
    def at(arr, x):
        f = (x - lo) / length * BINS - 0.5
        i = max(0, min(BINS - 1, int(math.floor(f))))
        j = min(BINS - 1, i + 1)
        t = max(0.0, min(1.0, f - i))
        return arr[i] * (1 - t) + arr[j] * t

    # Saw where the section is whole: from each end inwards until every sector around the axis has wood in it.
    trim_hi = end_trim(pts, hi, -1.0, cy, cz, mean_r, at)
    trim_lo = end_trim(pts, lo, 1.0, cy, cz, mean_r, at)
    usable = length - trim_hi - trim_lo
    modules = max(2, int(round(usable / MODULE)))
    new_length = modules * MODULE
    mid = (lo + trim_lo + hi - trim_hi) / 2.0
    scale_x = new_length / usable
    def move(x, y, z):
        y0, z0, r0 = at(cy, x), at(cz, x), at(rad, x)
        # Radial: whole log to the target diameter, then this slice's radius part of the way to it.
        local = scale_r * ((1 - ROUND) + ROUND * target_r / max(1e-3, scale_r * r0))
        ny = (1 - STRAIGHTEN) * (y0 - mean_y) + (y - y0) * local
        nz = (1 - STRAIGHTEN) * (z0 - mean_z) + (z - z0) * local
        return unreal.Vector((x - mid) * scale_x, ny, nz)

    grain = end_grain_maps(mesh, lo, hi, move)
    grain_material = source.get_editor_property('static_materials')[0].get_editor_property('material_interface')
    if key in END_GRAIN_FROM and END_GRAIN_FROM[key] in GRAIN:
        grain, grain_material = GRAIN[END_GRAIN_FROM[key]]
    GRAIN[key] = (grain, grain_material)
    out = [move(x, y, z) for x, y, z in pts]
    unreal.GeometryScript_MeshEdits.set_all_mesh_vertex_positions(mesh, array_to_vector_list(out))
    saw_ends(mesh, new_length)
    textured = texture_caps(mesh, grain)

    asset_path = '{}/SM_LogFit_{}'.format(OUT_DIR, source.get_name().split('_')[0])
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.EditorAssetLibrary.delete_asset(asset_path)
    options = unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
    options.enable_recompute_normals = False
    options.enable_recompute_tangents = False
    options.enable_nanite = True
    options.enable_collision = False
    result = unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(mesh, asset_path, options)
    fitted = next(r for r in (result if isinstance(result, tuple) else (result,)) if isinstance(r, unreal.StaticMesh))
    materials = list(source.get_editor_property('static_materials'))[:1]
    cut = unreal.StaticMaterial()
    cut.set_editor_property('material_interface', unreal.load_asset(CUT_MATERIAL))
    cut.set_editor_property('material_slot_name', 'CutEnd')
    grain_slot = unreal.StaticMaterial()
    grain_slot.set_editor_property('material_interface', grain_material)
    grain_slot.set_editor_property('material_slot_name', 'EndGrain')
    fitted.set_editor_property('static_materials', materials + [cut, grain_slot])
    # Nanite like the source mesh (the creation option alone does not keep it).
    nanite = fitted.get_editor_property('nanite_settings')
    nanite.set_editor_property('enabled', True)
    fitted.set_editor_property('nanite_settings', nanite)
    try:
        unreal.EditorStaticMeshLibrary.add_simple_collisions(fitted, unreal.ScriptCollisionShapeType.CAPSULE)
    except Exception as ex:
        log('collision', key, ex)
    unreal.EditorAssetLibrary.save_loaded_asset(fitted)

    bend = max(math.hypot(a - mean_y, b - mean_z) for a, b in zip(cy, cz))
    log(key, 'end grain from the scan on', textured, 'of 2 caps', {e: round(g[1], 3) for e, g in grain.items()})
    log('{}: {} {:.0f} -> {:.0f} cm ({} modules, sawn {:.0f} + {:.0f} cm off the ragged ends, stretch {:.2f}), '
        'diameter {:.0f} -> {:.0f} cm, bend {:.1f} -> {:.1f} cm'.format(
            key, source.get_name(), length, new_length, modules, trim_lo, trim_hi, scale_x, 2 * mean_r, TARGET_D,
            bend, bend * (1 - STRAIGHTEN)))
    return fitted, [(v.x, v.y, v.z) for v in out], new_length


SECTORS = 12
# Scan vertices are sparse (a couple per cm): each test looks at a window this long.
SAW_WINDOW = 10.0
SAW_MARGIN = 2.0


def end_trim(pts, end, inward, cy, cz, mean_r, at, step=1.0, limit=80.0):
    """How far in from a ragged end the cross-section is whole (wood in every sector around the axis), plus a margin."""
    depth = 0.0
    while depth < limit:
        a, b = sorted((end + inward * depth, end + inward * (depth + SAW_WINDOW)))
        sectors = set()
        for x, y, z in pts:
            if a <= x <= b:
                dy, dz = y - at(cy, x), z - at(cz, x)
                if math.hypot(dy, dz) > 0.6 * mean_r:
                    sectors.add(int((math.atan2(dz, dy) + math.pi) / (2 * math.pi) * SECTORS) % SECTORS)
        if len(sectors) == SECTORS:
            # Whole over the window: saw at its inner edge, where the rim is surely closed.
            return depth + SAW_WINDOW + SAW_MARGIN
        depth += step
    return limit


def saw_ends(mesh, length):
    """Cuts both ends flat at +-length/2 and caps them with a flat end-grain face (material 1).

    A plane cut drops the side its plane normal (Z) faces. The scanned log is an open shell whose cut rim is not a clean
    loop, so the cut's own hole fill leaves it hollow; the cap is built from the rim points instead."""
    options = unreal.GeometryScriptMeshPlaneCutOptions()
    options.fill_holes = False
    for x, pitch in ((length / 2.0, -90.0), (-length / 2.0, 90.0)):
        frame = unreal.Transform()
        frame.translation = unreal.Vector(x, 0.0, 0.0)
        frame.rotation = unreal.Rotator(roll=0.0, pitch=pitch, yaw=0.0).quaternion()
        unreal.GeometryScript_MeshBooleans.apply_mesh_plane_cut(mesh, frame, options)
    result = unreal.GeometryScript_MeshQueries.get_all_vertex_positions(mesh, False)
    positions = next(r for r in (result if isinstance(result, tuple) else (result,)) if isinstance(r, unreal.GeometryScriptVectorList))
    pts = [(p.x, p.y, p.z) for p in vector_list_to_array(positions)]
    for end, pitch in ((1.0, -90.0), (-1.0, 90.0)):
        cap_end(mesh, pts, end * length / 2.0, pitch)
    # The bark keeps the scan's own normals and tangents (its normal map was baked against them); the caps come with
    # their own flat normals.
    box = unreal.GeometryScript_MeshQueries.get_mesh_bounding_box(mesh)
    if abs((box.max.x - box.min.x) - length) > 0.5:
        raise RuntimeError('Sawn log is {:.1f} cm, expected {:.1f}'.format(box.max.x - box.min.x, length))


END_GRAIN_REGION = 30.0
END_GRAIN_MAX_ERROR = 0.03
# Half-width (UV) of the island kept around the busiest 0.1 UV cell of an end.
END_GRAIN_ISLAND = 0.15


def pick(result, kind):
    items = result if isinstance(result, tuple) else (result,)
    return [r for r in items if isinstance(r, kind)]


def solve3(m, v):
    """3 x 3 linear solve (Cramer)."""
    def det(a):
        return (a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0])
                + a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]))
    d = det(m)
    if abs(d) < 1e-9:
        return None
    out = []
    for c in range(3):
        mc = [row[:] for row in m]
        for r in range(3):
            mc[r][c] = v[r]
        out.append(det(mc) / d)
    return out


def affine_uv(samples):
    """Least-squares u, v = a*y + b*z + c over (y, z, u, v) samples; returns (coeffs_u, coeffs_v, rms error)."""
    if len(samples) < 6:
        return None
    ata = [[0.0] * 3 for _ in range(3)]
    atu = [0.0] * 3
    atv = [0.0] * 3
    for y, z, u, v in samples:
        row = (y, z, 1.0)
        for i in range(3):
            atu[i] += row[i] * u
            atv[i] += row[i] * v
            for j in range(3):
                ata[i][j] += row[i] * row[j]
    cu, cv = solve3(ata, atu), solve3(ata, atv)
    if cu is None or cv is None:
        return None
    err = (sum((cu[0] * y + cu[1] * z + cu[2] - u) ** 2 + (cv[0] * y + cv[1] * z + cv[2] - v) ** 2
               for y, z, u, v in samples) / len(samples)) ** 0.5
    return cu, cv, err


def end_grain_maps(mesh, lo, hi, move):
    """From the scan's own end faces (facing along the log, near its ends): a map from a point on the new cap to the
    texture's end-grain picture. {+1/-1: ((coeffs_u, coeffs_v), error)} for each end that has one."""
    queries = unreal.GeometryScript_MeshQueries
    triangles = unreal.GeometryScript_List.convert_index_list_to_array(pick(queries.get_all_triangle_i_ds(mesh), unreal.GeometryScriptIndexList)[0])
    faces = {1.0: [], -1.0: []}
    for tid in triangles:
        normal = pick(queries.get_triangle_face_normal(mesh, tid), unreal.Vector)[0]
        if abs(normal.x) < 0.85:
            continue
        corners = pick(queries.get_triangle_positions(mesh, tid), unreal.Vector)
        uvs = pick(queries.get_triangle_u_vs(mesh, 0, tid), unreal.Vector2D)
        if len(corners) < 3 or len(uvs) < 3:
            continue
        cx = sum(c.x for c in corners) / 3.0
        # Which end by position: the winding of a scan's faces is not to be trusted.
        end = 1.0 if cx > (lo + hi) / 2.0 else -1.0
        if (end > 0 and cx < hi - END_GRAIN_REGION) or (end < 0 and cx > lo + END_GRAIN_REGION):
            continue
        faces[end].append(([move(c.x, c.y, c.z) for c in corners], uvs))
    maps = {}
    for end, found in faces.items():
        # Only the end's main UV island: faces near the ends that face along the log also include bark slivers whose
        # UVs lie elsewhere in the texture.
        cells = {}
        for corners, uvs in found:
            cell = (round(sum(u.x for u in uvs) / 3.0, 1), round(sum(u.y for u in uvs) / 3.0, 1))
            cells[cell] = cells.get(cell, 0) + 1
        if not cells:
            continue
        seed = max(cells, key=cells.get)
        samples = []
        for corners, uvs in found:
            cu, cv = sum(u.x for u in uvs) / 3.0, sum(u.y for u in uvs) / 3.0
            if abs(cu - seed[0]) <= END_GRAIN_ISLAND and abs(cv - seed[1]) <= END_GRAIN_ISLAND:
                samples += [(c.y, c.z, u.x, u.y) for c, u in zip(corners, uvs)]
        fit = affine_uv(samples)
        if fit:
            maps[end] = ((fit[0], fit[1]), fit[2])
    return maps


def texture_caps(mesh, grain):
    """Caps whose end has a good map take the scan's end grain (material 2); the others keep cut wood (material 1)."""
    materials = unreal.GeometryScript_Materials
    queries = unreal.GeometryScript_MeshQueries
    caps = unreal.GeometryScript_List.convert_index_list_to_array(pick(materials.get_triangles_by_material_id(mesh, 1), unreal.GeometryScriptIndexList)[0])
    done = set()
    for tid in caps:
        corners = pick(queries.get_triangle_positions(mesh, tid), unreal.Vector)
        end = 1.0 if sum(c.x for c in corners) > 0 else -1.0
        # This end's own picture if it fits well, else the other end's (a rough broken end has no clean one).
        usable = {e: g for e, g in grain.items() if g[1] <= END_GRAIN_MAX_ERROR}
        if not usable:
            continue
        source = end if end in usable else min(usable, key=lambda e: usable[e][1])
        (cu, cv), _ = usable[source]
        tri = unreal.GeometryScriptUVTriangle()
        for name, c in zip(('uv0', 'uv1', 'uv2'), corners):
            tri.set_editor_property(name, unreal.Vector2D(cu[0] * c.y + cu[1] * c.z + cu[2], cv[0] * c.y + cv[1] * c.z + cv[2]))
        unreal.GeometryScript_UVs.set_mesh_triangle_u_vs(mesh, 0, tid, tri)
        materials.set_triangle_material_id(mesh, tid, 2)
        done.add(end)
    return len(done)


def triangle_id_count(mesh):
    queries = unreal.GeometryScript_MeshQueries
    for name in ('get_num_triangle_i_ds', 'get_num_triangle_ids'):
        if hasattr(queries, name):
            return getattr(queries, name)(mesh)
    raise RuntimeError('No triangle ID count in this engine version')


def convex_hull(points):
    pts = sorted(set((round(a, 3), round(b, 3)) for a, b in points))

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]


def cap_end(mesh, pts, x, pitch):
    """A flat polygon over the rim at x, facing out of the log, given material 1 (end grain)."""
    rim = [(y, z) for px, y, z in pts if abs(px - x) < 0.05]
    if len(rim) < 3:
        raise RuntimeError('No cut rim at x={:.1f}'.format(x))
    # The rim zigzags where the scan is rough; its convex hull is a clean disc (the log's section is nearly round).
    rim = convex_hull(rim)
    # Polygon in the cap frame: with pitch -90 (the +X end) local X is world -Z and local Y world +Y; with pitch +90
    # local X is world +Z. The frame's Z is the cap's outward normal, so the polygon must run counter-clockwise.
    sign = -1.0 if pitch < 0 else 1.0
    poly = [(sign * z, y) for y, z in rim]
    area = sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(poly, poly[1:] + poly[:1]))
    if area < 0:
        poly.reverse()
    frame = unreal.Transform()
    frame.translation = unreal.Vector(x, 0.0, 0.0)
    frame.rotation = unreal.Rotator(roll=0.0, pitch=pitch, yaw=0.0).quaternion()
    options = unreal.GeometryScriptPrimitiveOptions()
    options.material_id = 1
    unreal.GeometryScript_Primitives.append_triangulated_polygon(
        mesh, options, frame, [unreal.Vector2D(u, v) for u, v in poly], True)
    # The polygon coordinates above are used to build geometry in centimetres. Give the cut
    # faces their own planar UVs as well, otherwise the cut-wood material samples an enormous
    # range and its end-grain pattern disappears at this scale.
    cap_ids = unreal.GeometryScript_List.convert_index_list_to_array(
        pick(unreal.GeometryScript_Materials.get_triangles_by_material_id(mesh, 1), unreal.GeometryScriptIndexList)[0])
    queries = unreal.GeometryScript_MeshQueries
    for tid in cap_ids:
        corners = pick(queries.get_triangle_positions(mesh, tid), unreal.Vector)
        if len(corners) < 3 or any(abs(c.x - x) > 0.05 for c in corners):
            continue
        uv_tri = unreal.GeometryScriptUVTriangle()
        for name, c in zip(('uv0', 'uv1', 'uv2'), corners):
            uv_tri.set_editor_property(name, unreal.Vector2D(0.5 + c.y / CUT_UV_CM, 0.5 + c.z / CUT_UV_CM))
        unreal.GeometryScript_UVs.set_mesh_triangle_u_vs(mesh, 0, tid, uv_tri)


# ----------------------------------------------------------------------------- fitting

def surface_samples(asset, per_edge=4):
    """Points spread over every triangle of the fitted mesh: vertices alone are too sparse to show a slice's underside."""
    pts = []
    for section in range(asset.get_num_sections(0)):
        verts, tris = unreal.ProceduralMeshLibrary.get_section_from_static_mesh(asset, 0, section)[:2]
        v = [(p.x, p.y, p.z) for p in verts]
        for i in range(0, len(tris) - 2, 3):
            a, b, c = v[tris[i]], v[tris[i + 1]], v[tris[i + 2]]
            for u in range(per_edge + 1):
                for w in range(per_edge + 1 - u):
                    s1, s2 = u / per_edge, w / per_edge
                    s0 = 1.0 - s1 - s2
                    pts.append(tuple(s0 * a[k] + s1 * b[k] + s2 * c[k] for k in range(3)))
    return pts


def rotate(rotator, v):
    if hasattr(rotator, 'rotate_vector'):
        return rotator.rotate_vector(v)
    return unreal.MathLibrary.greater_greater_vector_rotator(v, rotator)


def pose_points(pts, roll, flip):
    """Log-space points rotated by the engine exactly as a placed actor with this roll and yaw will be."""
    rotator = unreal.Rotator(roll=roll, pitch=0.0, yaw=180.0 if flip else 0.0)
    out = []
    for x, y, z in pts:
        v = rotate(rotator, unreal.Vector(x, y, z))
        out.append((v.x, v.y, v.z))
    return out


# Slices along the log; the first and last are the rough sawn ends and do not bear on the course below.
ENVELOPE_BINS = 26
END_BINS = 1


def envelopes(pts, length, bins=ENVELOPE_BINS, axis=2):
    """Lowest and highest point of each slice along the log on one axis: z gives the faces that meet the courses
    below and above, y the sides that meet the logs lying next to it."""
    low = [None] * bins
    high = [None] * bins
    for p in pts:
        x, z = p[0], p[axis]
        i = min(bins - 1, max(0, int((x / length + 0.5) * bins)))
        low[i] = z if low[i] is None else min(low[i], z)
        high[i] = z if high[i] is None else max(high[i], z)
    for arr in (low, high):
        known = [i for i, v in enumerate(arr) if v is not None]
        for i in range(bins):
            if arr[i] is None:
                arr[i] = arr[min(known, key=lambda k: abs(k - i))]
    return low[END_BINS:bins - END_BINS], high[END_BINS:bins - END_BINS]


POSES = [(roll, flip) for flip in (False, True) for roll in (0, 90, 180, 270)]


def fit_courses(pts, length, base_z, height, first_offset=0.0):
    """Courses of one log, each in the pose that fits the course below best. Returns [(z, roll, flip, sink, spread)]."""
    shapes = {}
    for roll, flip in POSES:
        shapes[(roll, flip)] = envelopes(pose_points(pts, roll, flip), length)
    courses = []
    below_top = None
    previous = None
    while True:
        best = None
        for pose, (low, high) in shapes.items():
            if pose == previous:
                continue  # the same pose twice in a row repeats the same bumps
            if below_top is None:
                z = base_z - min(low) - GROUND_SINK + first_offset
                gap = [0.0]
            else:
                # Sink: how far this log reaches into the course below at each slice once lowered onto it.
                room = [t - b for t, b in zip(below_top, low)]
                z = min(room) - MIN_OVERLAP
                gap = [r - min(room) for r in room]
            spread = max(gap) - min(gap)
            if best is None or spread < best[1]:
                best = (pose, spread, z, high)
        pose, spread, z, high = best
        if courses and z + max(high) > base_z + height + TARGET_D * 0.5:
            break
        courses.append((z, pose[0], pose[1], round(MIN_OVERLAP + spread, 1) if courses else GROUND_SINK, round(spread, 1)))
        below_top = [z + h for h in high]
        previous = pose
        if z + max(high) >= base_z + height:
            break
    return courses


# ----------------------------------------------------------------------------- scene

def ground_top():
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if actor.get_actor_label() == 'Ground':
            origin, extent = actor.get_actor_bounds(False)
            return origin.z + extent.z
    return 0.0


def clear_walls():
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    gone = 0
    for actor in actors.get_all_level_actors():
        label = actor.get_actor_label()
        if label.startswith('LogWall') or label.startswith('Sign_LogWall'):
            actors.destroy_actor(actor)
            gone += 1
    log('removed', gone, 'old wall actors')


def place(mesh, label, folder, location, roll, yaw):
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location),
                                          unreal.Rotator(roll=roll, pitch=0.0, yaw=yaw))
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    return actor


SIGN_FOLD = str.maketrans({'İ': 'I', 'ı': 'i', 'Ş': 'S', 'ş': 's', 'Ğ': 'G', 'ğ': 'g'})


def sign(text, location, yaw, size=24.0):
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*location), unreal.Rotator(0, 0, yaw))
    actor.set_actor_label('Sign_LogWall')
    actor.set_folder_path('LogWallFit/Signs')
    comp = actor.get_editor_property('text_render')
    comp.set_editor_property('text', text.translate(SIGN_FOLD))
    comp.set_editor_property('world_size', size)
    comp.set_editor_property('horizontal_alignment', unreal.HorizTextAligment.EHTA_CENTER)


def build_wall(key, mesh, pts, length, y, base_z, report):
    courses = fit_courses(pts, length, base_z, WALL_HEIGHT)
    for index, (z, roll, flip, sink, spread) in enumerate(courses):
        place(mesh, 'LogWallFit_{}_c{}'.format(key, index), 'LogWallFit/' + key,
              (length / 2.0, y, z), roll, 180.0 if flip else 0.0)
    steps = [round(b[0] - a[0], 1) for a, b in zip(courses, courses[1:])]
    report[key] = dict(length=length, courses=len(courses), steps=steps,
                       poses=[(c[1], c[2]) for c in courses], sink=[c[3] for c in courses])
    sign('{}: {:.0f} cm, {} sıra'.format(key, length, len(courses)), (length / 2.0, y - 60.0, base_z + 290.0), -90.0)
    log('wall', key, len(courses), 'courses, steps', steps)


def build_corner(mesh, pts, length, y, base_z, report):
    """Two walls crossing at (0, y): one along X, one along Y half a course higher, ends running past the corner."""
    along_x = fit_courses(pts, length, base_z, WALL_HEIGHT)
    step = along_x[1][0] - along_x[0][0] if len(along_x) > 1 else TARGET_D * 0.8
    # The second wall starts half a course lower: its first log is half sunk, like a cabin's half-log sill.
    along_y = fit_courses(pts, length, base_z, WALL_HEIGHT, first_offset=-step / 2.0)
    centre = length / 2.0 - CORNER_RUNOUT
    for index, (z, roll, flip, _, _) in enumerate(along_x):
        place(mesh, 'LogWallFit_CornerX_c{}'.format(index), 'LogWallFit/Corner', (centre, y, z), roll, 180.0 if flip else 0.0)
    for index, (z, roll, flip, _, _) in enumerate(along_y):
        place(mesh, 'LogWallFit_CornerY_c{}'.format(index), 'LogWallFit/Corner', (0.0, y + centre, z), roll, 270.0 if flip else 90.0)
    report['corner'] = dict(x_courses=len(along_x), y_courses=len(along_y), half_step=round(step / 2.0, 1))
    sign('köşe: yarım sıra kaydırmalı', (centre, y - 60.0, base_z + 290.0), -90.0)
    log('corner', len(along_x), '+', len(along_y), 'courses')


def build_floor(key, mesh, pts, length, y0, base_z, report):
    """Logs lying side by side towards +Y from y0, each in the pose whose side follows its neighbour's most evenly."""
    shapes = {}
    for roll, flip in POSES:
        moved = pose_points(pts, roll, flip)
        shapes[(roll, flip)] = (envelopes(moved, length, axis=1), envelopes(moved, length, axis=2))
    previous, side, last_y = None, None, None
    spacing, sinks = [], []
    for index in range(FLOOR_LOGS):
        best = None
        for pose, ((left, right), (low, _)) in shapes.items():
            if pose == previous:
                continue
            if side is None:
                y, spread = y0 - min(left), 0.0
            else:
                # How far this log reaches into its neighbour at each slice once pushed against it.
                room = [r - l for r, l in zip(side, left)]
                y, spread = min(room) - MIN_OVERLAP, max(room) - min(room)
            if best is None or spread < best[1]:
                best = (pose, spread, y, right, low)
        pose, spread, y, right, low = best
        # Every log rests on the ground on its own lowest point.
        z = base_z - min(low) - GROUND_SINK
        place(mesh, 'LogWallFit_Floor_{}_{}'.format(key, index), 'LogWallFit/Floor_' + key,
              (length / 2.0, y, z), pose[0], 180.0 if pose[1] else 0.0)
        if last_y is not None:
            spacing.append(round(y - last_y, 1))
            sinks.append(round(MIN_OVERLAP + spread, 1))
        last_y = y
        side = [y + r for r in right]
        previous = pose
    report['floor_' + key] = dict(logs=FLOOR_LOGS, spacing=spacing, sink=sinks)
    log('floor', key, FLOOR_LOGS, 'logs, spacing', spacing, 'sink', sinks)


def build_block(key, mesh, pts, length, y_far, base_z, report):
    """BLOCK x BLOCK x BLOCK logs; returns the y of the block's far side (towards -Y) for the next block."""
    shapes = {}
    for roll, flip in POSES:
        moved = pose_points(pts, roll, flip)
        shapes[(roll, flip)] = (envelopes(moved, length, axis=1), envelopes(moved, length, axis=2))
    count = 0
    y_steps, z_steps, sinks = [], [], []
    min_y = None
    for slot in range(BLOCK):
        x = length / 2.0 + slot * length
        # Per column (side by side, towards -Y) the top of the log below; per layer the side of the log beside.
        below = [None] * BLOCK
        for layer in range(BLOCK):
            beside = None
            for column in range(BLOCK):
                best = None
                for pose, ((left, right), (low, high)) in shapes.items():
                    under = below[column]
                    if under is not None and pose == under[3]:
                        continue  # the same pose straight above repeats the same bumps
                    if under is None:
                        z, spread_z = base_z - min(low) - GROUND_SINK, 0.0
                    else:
                        room = [t - b for t, b in zip(under[0], low)]
                        z, spread_z = min(room) - MIN_OVERLAP, max(room) - min(room)
                    if beside is None:
                        y, spread_y = y_far - max(right), 0.0
                    else:
                        # Columns run towards -Y: this log's right side meets the left side of the one beside it.
                        room = [l - r for l, r in zip(beside, right)]
                        y, spread_y = max(room) + MIN_OVERLAP, max(room) - min(room)
                        y = beside_y_ref + y
                    score = spread_z + spread_y
                    if best is None or score < best[0]:
                        best = (score, pose, z, y, low, high, left, spread_z, spread_y)
                score, pose, z, y, low, high, left, spread_z, spread_y = best
                place(mesh, 'LogWallFit_Block_{}_x{}_y{}_z{}'.format(key, slot, column, layer), 'LogWallFit/Block_' + key,
                      (x, y, z), pose[0], 180.0 if pose[1] else 0.0)
                count += 1
                if below[column] is not None:
                    z_steps.append(z - below[column][1])
                    sinks.append(MIN_OVERLAP + spread_z)
                if beside is not None:
                    y_steps.append(beside_y - y)
                below[column] = ([z + h for h in high], z, None, pose)
                beside = [y + l for l in left]
                beside_y, beside_y_ref = y, 0.0
                min_y = y + min(left) if min_y is None else min(min_y, y + min(left))

    def stats(values):
        return [round(min(values), 1), round(sum(values) / len(values), 1), round(max(values), 1)] if values else []
    report['block_' + key] = dict(logs=count, size_cm=[round(BLOCK * length), round(y_far - min_y), None],
                                  y_step_min_mean_max=stats(y_steps), z_step_min_mean_max=stats(z_steps),
                                  sink_min_mean_max=stats(sinks))
    sign('{}: {} x {} x {}'.format(key, BLOCK, BLOCK, BLOCK), (length / 2.0, y_far + 60.0, base_z + 300.0), 90.0, size=40.0)
    log('block', key, count, 'logs, y step', stats(y_steps), 'z step', stats(z_steps), 'sink', stats(sinks))
    return min_y


def joint_faces(pts, length):
    """Socket offsets of the six faces, from the log stacked and laid beside itself in its plain pose."""
    (left, right), (low, high) = envelopes(pts, length, axis=1), envelopes(pts, length, axis=2)
    room_z = [h - l for h, l in zip(high, low)]
    room_y = [r - l for r, l in zip(right, left)]
    dz = min(room_z) - MIN_OVERLAP
    dy = min(room_y) - MIN_OVERLAP
    ex = length / 2.0 - END_TOUCH
    offsets = {'Top': (0.0, 0.0, dz / 2.0), 'Bottom': (0.0, 0.0, -dz / 2.0),
               'SideR': (0.0, dy / 2.0, 0.0), 'SideL': (0.0, -dy / 2.0, 0.0),
               'EndA': (ex, 0.0, 0.0), 'EndB': (-ex, 0.0, 0.0)}
    # Each socket's X axis points out of its face.
    rotations = {'Top': unreal.Rotator(roll=0.0, pitch=90.0, yaw=0.0), 'Bottom': unreal.Rotator(roll=0.0, pitch=-90.0, yaw=0.0),
                 'SideR': unreal.Rotator(roll=0.0, pitch=0.0, yaw=90.0), 'SideL': unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0),
                 'EndA': unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0), 'EndB': unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0)}
    sink = {'Top': MIN_OVERLAP + max(room_z) - min(room_z), 'SideR': MIN_OVERLAP + max(room_y) - min(room_y), 'EndA': END_TOUCH}
    for face in ('Bottom', 'SideL', 'EndB'):
        sink[face] = sink[OPPOSITE[face]]
    return offsets, rotations, sink


def add_sockets(asset, offsets, rotations):
    sockets = []
    for face, (x, y, z) in offsets.items():
        socket = unreal.StaticMeshSocket(asset)
        socket.set_editor_property('socket_name', 'Snap_' + face)
        socket.set_editor_property('relative_location', unreal.Vector(x, y, z))
        socket.set_editor_property('relative_rotation', rotations[face])
        sockets.append(socket)
    if hasattr(asset, 'add_socket'):
        for socket in sockets:
            asset.add_socket(socket)
    else:
        raise RuntimeError('StaticMesh.add_socket is not available')
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return [face for face in offsets if asset.find_socket('Snap_' + face) is not None]


def add_arrow(actor, name, location, rotation, colour):
    """An editor-only arrow component on a placed actor (the way the Main logs carry their Snap_* faces)."""
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    root = subsystem.k2_gather_subobject_data_for_instance(actor)[0]
    params = unreal.AddNewSubobjectParams(parent_handle=root, new_class=unreal.ArrowComponent, blueprint_context=None)
    handle, failure = subsystem.add_new_subobject(params)
    if not unreal.SubobjectDataBlueprintFunctionLibrary.is_handle_valid(handle):
        raise RuntimeError('Could not add {} to {}: {}'.format(name, actor.get_actor_label(), failure))
    subsystem.rename_subobject(handle, unreal.Text(name))
    arrow = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle))
    arrow.set_editor_property('relative_location', unreal.Vector(*location))
    arrow.set_editor_property('relative_rotation', rotation)
    arrow.set_editor_property('arrow_length', ARROW_LENGTH)
    arrow.set_editor_property('arrow_color', unreal.Color(r=colour[0], g=colour[1], b=colour[2], a=255))
    return arrow


def add_face_arrows(actor, offsets, rotations, faces=None, green=()):
    """Snap_<Face> arrows at the measured faces (all six unless faces is given); the faces in green are drawn green."""
    for face, _ in FACES:
        if faces is not None and face not in faces:
            continue
        add_arrow(actor, 'Snap_' + face, offsets[face], rotations[face], ARROW_GREEN if face in green else ARROW_RED)
    actor.set_editor_property('tags', list(actor.get_editor_property('tags')) + ['SnapLog'])


def build_reference(key, mesh, length, faces, x, y, z):
    """One log alone with all six face arrows and the face names beside them."""
    offsets, rotations, _ = faces
    actor = place(mesh, 'LogWallFit_Faces_' + key, 'LogWallFit/Joints_' + key, (x, y, z), 0.0, 0.0)
    add_face_arrows(actor, offsets, rotations)
    for face, name in FACES:
        o = offsets[face]
        # The name a little past the arrow tip, out along the face.
        direction = [0.0 if abs(v) < 1e-3 else (1.0 if v > 0 else -1.0) for v in o]
        tip = [x + o[0] + direction[0] * (ARROW_LENGTH + 20.0), y + o[1] + direction[1] * (ARROW_LENGTH + 20.0),
               z + o[2] + direction[2] * (ARROW_LENGTH + 20.0)]
        sign(name, (tip[0], tip[1] + 25.0, tip[2]), 90.0, size=18.0)
    sign('{} · 6 yüz'.format(key), (x, y + 60.0, z + 130.0), 90.0, size=30.0)


def build_joints(fitted, faces, base_z, report):
    """One pair per log and face: the other log joined face to opposite face, a marker at the joint."""
    sphere = unreal.load_asset('/Engine/BasicShapes/Sphere')
    joints = []
    for key, row_y in JOINT_ROWS.items():
        mesh, pts, length = fitted[key]
        offsets, _, sink = faces[key]
        low = envelopes(pts, length)[0]
        rest_z = base_z - min(low) - GROUND_SINK
        build_reference(key, mesh, length, faces[key], length / 2.0, row_y, rest_z + 60.0)
        cursor = length + JOINT_GAP + 100.0
        for face, name in FACES:
            fo, oo = offsets[face], offsets[OPPOSITE[face]]
            other = tuple(a - b for a, b in zip(fo, oo))
            # The lower of the two logs rests on the ground; the pair starts at the cursor along X.
            lift = rest_z - min(0.0, other[2])
            bx = cursor + length / 2.0 - min(0.0, other[0])
            by = row_y - max(0.0, other[1])
            base = (bx, by, lift)
            joined = (bx + other[0], by + other[1], lift + other[2])
            a_label = 'LogWallFit_Joint_{}_{}_A'.format(key, face)
            b_label = 'LogWallFit_Joint_{}_{}_B'.format(key, face)
            a = place(mesh, a_label, 'LogWallFit/Joints_' + key, base, 0.0, 0.0)
            b = place(mesh, b_label, 'LogWallFit/Joints_' + key, joined, 0.0, 0.0)
            a.set_editor_property('tags', ['Joint:{}->{}:{}'.format(face, b_label, OPPOSITE[face])])
            b.set_editor_property('tags', ['Joint:{}->{}:{}'.format(OPPOSITE[face], a_label, face)])
            # A shows its six faces, the joined one green; B only the face it joins by, green: the two meet.
            add_face_arrows(a, offsets, faces[key][1], green=(face,))
            add_face_arrows(b, offsets, faces[key][1], faces=(OPPOSITE[face],), green=(OPPOSITE[face],))
            top = max(base[2], joined[2]) + TARGET_D
            sign('{} · {}'.format(key, name), ((min(bx, joined[0]) + max(bx, joined[0])) / 2.0, row_y + 60.0, top + 40.0), 90.0, size=30.0)
            joints.append(dict(a=a_label, face_a=face, b=b_label, face_b=OPPOSITE[face],
                               offset_cm=[round(v, 1) for v in other], sink_max_cm=round(sink[face], 1)))
            cursor = max(bx, joined[0]) + length / 2.0 + JOINT_GAP
        log('joints', key, 'faces', {f: [round(v, 1) for v in offsets[f]] for f, _ in FACES})
    report['joints'] = joints


def build_crib(key, mesh, pts, length, x0, y0, base_z, report):
    """2 logs along X and 2 along Y half a course higher, crossing at the four corners of a square."""
    courses = fit_courses(pts, length, base_z, TARGET_D * 2.0)
    step = courses[1][0] - courses[0][0] if len(courses) > 1 else TARGET_D * 0.8
    side = length - 2.0 * CORNER_RUNOUT
    centre = length / 2.0 - CORNER_RUNOUT
    z_x, roll_x, flip_x = courses[0][:3]
    z_y = z_x + step / 2.0
    others = [p for p in POSES if p != (roll_x, flip_x)]
    for index, yy in enumerate((y0, y0 + side)):
        roll, flip = (roll_x, flip_x) if index == 0 else others[0]
        place(mesh, 'LogWallFit_Crib_{}_X{}'.format(key, index), 'LogWallFit/Crib_' + key,
              (x0 + centre, yy, z_x), roll, 180.0 if flip else 0.0)
    for index, xx in enumerate((x0, x0 + side)):
        roll, flip = others[1 + index]
        place(mesh, 'LogWallFit_Crib_{}_Y{}'.format(key, index), 'LogWallFit/Crib_' + key,
              (xx, y0 + centre, z_y), roll, 270.0 if flip else 90.0)
    report['crib_' + key] = dict(side=round(side, 1), half_step=round(step / 2.0, 1),
                                 notch_depth=round(TARGET_D - step / 2.0, 1))
    sign('{}: 2X + 2Y'.format(key), (x0 + side / 2.0, y0 - 60.0, base_z + 150.0), -90.0)
    log('crib', key, 'side', round(side), 'half step', round(step / 2.0, 1))


def main():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if not world or 'LogWallTest' not in world.get_path_name():
        les.load_level(MAP)
    unreal.EditorAssetLibrary.make_directory(OUT_DIR)

    # Walls first: a rerun replaces meshes the old walls still use.
    clear_walls()
    fitted = {}
    for key, folder in LOGS.items():
        path = best_log_mesh(folder)
        log(key, 'source', path)
        asset, _, length = normalise(key, path)
        fitted[key] = (asset, surface_samples(asset), length)
        log(key, 'surface samples', len(fitted[key][1]))
    base_z = ground_top()
    report = dict(target_d=TARGET_D, ground=base_z)
    faces = {}
    for key, (asset, pts, length) in fitted.items():
        offsets, rotations, sink = joint_faces(pts, length)
        faces[key] = (offsets, rotations, sink)
        report['sockets_' + key] = add_sockets(asset, offsets, rotations)
    build_joints(fitted, faces, base_z, report)
    for key, y in WALL_Y.items():
        mesh, pts, length = fitted[key]
        build_floor(key, mesh, pts, length, y + FLOOR_GAP, base_z, report)
        build_crib(key, mesh, pts, length, CRIB_X, CRIB_Y[key], base_z, report)
    mesh, pts, length = fitted[CORNER_LOG]
    build_corner(mesh, pts, length, CORNER_Y, base_z, report)

    les.save_current_level()
    with open(REPORT, 'w', encoding='utf-8') as f:
        json.dump(dict(report=report, log=LOG), f, indent=1, ensure_ascii=False)
    log('done')


main()
