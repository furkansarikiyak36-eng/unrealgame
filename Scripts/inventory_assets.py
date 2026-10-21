"""Cost inventory of every asset under /Game, from asset registry tags (nothing is loaded).

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/inventory_assets.py"
Writes Saved/asset_inventory.json (one row per asset) and Saved/asset_inventory.csv.
Per asset: class, pack (first folder under /Game, or /Game/Fab/Megascans/<kind>/<name>), disk size of the .uasset/.ubulk,
and for
  StaticMesh / SkeletalMesh: triangles and vertices of LOD0 (the fallback mesh for Nanite), Nanite triangles, LODs, bones
  Texture2D: dimensions, pixel format, LOD group, streaming, estimated GPU memory with mips
"""
import csv
import json
import os

import unreal

CONTENT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir())
# Bytes per pixel of the common GPU formats (block formats per pixel).
BPP = {'PF_DXT1': 0.5, 'PF_BC4': 0.5, 'PF_DXT3': 1, 'PF_DXT5': 1, 'PF_BC5': 1, 'PF_BC7': 1, 'PF_BC6H': 1,
       'PF_G8': 1, 'PF_A8': 1, 'PF_G16': 2, 'PF_R16F': 2, 'PF_B8G8R8A8': 4, 'PF_R8G8B8A8': 4, 'PF_FloatRGBA': 8,
       'PF_A16B16G16R16': 8, 'PF_A32B32G32R32F': 16, 'PF_R32_FLOAT': 4}


def tag(d, name):
    try:
        v = d.get_tag_value(name)
        return None if v in (None, '') else str(v)
    except Exception:
        return None


def num(v):
    try:
        return float(str(v).replace(',', ''))
    except (TypeError, ValueError):
        return None


def disk_bytes(package):
    base = os.path.join(CONTENT, package[len('/Game/'):])
    total = 0
    for ext in ('.uasset', '.ubulk', '.uexp', '.umap'):
        p = base + ext
        if os.path.exists(p):
            total += os.path.getsize(p)
    return total


def pack_of(package):
    parts = package.split('/')[2:]          # after /Game
    if not parts:
        return '(root)'
    if parts[0] == 'Fab' and len(parts) > 3 and parts[1] == 'Megascans':
        return 'Fab/Megascans/{}/{}'.format(parts[2], parts[3])
    if parts[0] in ('Fab', 'Megaplant_Library') and len(parts) > 2:
        return '{}/{}'.format(parts[0], parts[1])
    return parts[0] if len(parts) > 1 else '(root)'


def main():
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    rows = []
    for d in reg.get_assets_by_path('/Game', recursive=True):
        cls = str(d.asset_class_path.asset_name)
        pkg = str(d.package_name)
        row = {'asset': str(d.asset_name), 'class': cls, 'package': pkg, 'pack': pack_of(pkg), 'disk_mb': round(disk_bytes(pkg) / 1048576.0, 3)}
        if cls in ('StaticMesh', 'SkeletalMesh'):
            row.update({'tris': num(tag(d, 'Triangles')), 'verts': num(tag(d, 'Vertices')), 'lods': num(tag(d, 'LODs')),
                        'nanite': tag(d, 'NaniteEnabled') == 'True', 'nanite_tris': num(tag(d, 'NaniteTriangles')),
                        'bones': num(tag(d, 'Bones'))})
        elif cls in ('Texture2D', 'TextureCube', 'VirtualTexture2D'):
            dims = tag(d, 'Dimensions') or ''
            fmt = tag(d, 'Format') or ''
            w = h = None
            if 'x' in dims:
                try:
                    w, h = [int(x) for x in dims.split('x')[:2]]
                except ValueError:
                    pass
            faces = 6 if cls == 'TextureCube' else 1
            est = round(w * h * BPP.get(fmt, 1) * 1.333 * faces / 1048576.0, 2) if w and h else None
            row.update({'dims': dims, 'format': fmt, 'lod_group': tag(d, 'LODGroup'), 'never_stream': tag(d, 'NeverStream'),
                        'compression': tag(d, 'CompressionSettings'), 'vt': tag(d, 'VirtualTextureStreaming'), 'gpu_mb_est': est})
        rows.append(row)
    out = os.path.join(unreal.Paths.project_saved_dir(), 'asset_inventory')
    with open(out + '.json', 'w') as f:
        json.dump(rows, f)
    keys = sorted({k for r in rows for k in r})
    with open(out + '.csv', 'w', newline='', encoding='utf-8') as f:
        w = csv.DictWriter(f, fieldnames=keys)
        w.writeheader()
        w.writerows(rows)
    unreal.log('[Inventory] {} assets'.format(len(rows)))


main()
