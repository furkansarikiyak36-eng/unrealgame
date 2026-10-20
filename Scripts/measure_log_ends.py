"""Measures the Fab logs' diameter along their length (read only: no asset or map is changed).

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/measure_log_ends.py"
Writes Saved/log_ends.json: per log the length, the diameter in BINS slices along X, the mean diameter, and the
diameter at each end where the section is whole (the ragged ends are skipped as build_logwall_fit.py saws them).
"""
import json
import math
import os

import unreal

FAB = '/Game/Fab/Megascans/3D'
LOGS = {
    'wcokfh1': 'Old_Wooden_Log_wcokfh1',
    'wcjheff': 'Old_Wooden_Log_wcjheff',
    'wcokfdy': 'Old_Wooden_Log_wcokfdy',
    'wcolfdz': 'Old_Wooden_Log_wcolfdz',
}
BINS = 24
SECTORS = 12
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'log_ends.json')


def best_mesh(folder):
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    for quality in ('High', 'Medium', 'Low'):
        found = [str(a.package_name) for a in registry.get_assets_by_path('{}/{}/{}'.format(FAB, folder, quality), recursive=True)
                 if str(a.asset_class_path.asset_name) == 'StaticMesh']
        if found:
            return sorted(found, key=lambda p: p.rsplit('_tier_', 1)[-1])[0]
    raise RuntimeError('No mesh for ' + folder)


def vector_list_to_array(vlist):
    for owner in ('GeometryScript_List', 'GeometryScript_ListUtilities'):
        lib = getattr(unreal, owner, None)
        if lib and hasattr(lib, 'convert_vector_list_to_array'):
            return lib.convert_vector_list_to_array(vlist)
    raise RuntimeError('No vector list conversion in this engine version')


def points(path):
    source = unreal.load_asset(path)
    mesh = unreal.DynamicMesh()
    lod = unreal.GeometryScriptMeshReadLOD()
    lod.lod_type = unreal.GeometryScriptLODType.SOURCE_MODEL
    result = unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(
        source, mesh, unreal.GeometryScriptCopyMeshFromAssetOptions(), lod)
    mesh = next(r for r in (result if isinstance(result, tuple) else (result,)) if isinstance(r, unreal.DynamicMesh))
    result = unreal.GeometryScript_MeshQueries.get_all_vertex_positions(mesh, False)
    positions = next(r for r in (result if isinstance(result, tuple) else (result,)) if isinstance(r, unreal.GeometryScriptVectorList))
    return [(p.x, p.y, p.z) for p in vector_list_to_array(positions)], mesh.get_triangle_count() if hasattr(mesh, 'get_triangle_count') else None


def measure(path):
    pts, tris = points(path)
    lo = min(p[0] for p in pts)
    hi = max(p[0] for p in pts)
    length = hi - lo
    groups = [[] for _ in range(BINS)]
    for p in pts:
        groups[min(BINS - 1, int((p[0] - lo) / length * BINS))].append(p)
    slices = []
    for g in groups:
        if len(g) < 6:
            slices.append(None)
            continue
        ys = [p[1] for p in g]
        zs = [p[2] for p in g]
        y0, z0 = (min(ys) + max(ys)) / 2.0, (min(zs) + max(zs)) / 2.0
        radii = [math.hypot(p[1] - y0, p[2] - z0) for p in g]
        mean_r = sum(radii) / len(radii)
        # Whole: wood found all round the axis (a ragged or broken end leaves sectors empty).
        sectors = {int((math.atan2(p[2] - z0, p[1] - y0) + math.pi) / (2 * math.pi) * SECTORS) % SECTORS
                   for p, r in zip(g, radii) if r > 0.6 * mean_r}
        slices.append({'d': round(2 * mean_r, 1), 'cy': round(y0, 1), 'cz': round(z0, 1), 'whole': len(sectors) == SECTORS})
    whole = [i for i, s in enumerate(slices) if s and s['whole']]
    ds = [s['d'] for s in slices if s]
    return {
        'mesh': path,
        'length': round(length, 1),
        'vertices': len(pts),
        'mean_d': round(sum(ds) / len(ds), 1),
        'end_lo': {'slice': whole[0], 'd': slices[whole[0]]['d']} if whole else None,
        'end_hi': {'slice': whole[-1], 'd': slices[whole[-1]]['d']} if whole else None,
        'bend': round(max(math.hypot(s['cy'] - slices[whole[0]]['cy'], s['cz'] - slices[whole[0]]['cz'])
                          for s in slices if s) if whole else 0.0, 1),
        'slices': slices,
    }


def main():
    report = {}
    for key, folder in LOGS.items():
        try:
            report[key] = measure(best_mesh(folder))
        except Exception as error:
            report[key] = {'error': str(error)}
    with open(OUT, 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[LogEnds] wrote ' + OUT)


main()
