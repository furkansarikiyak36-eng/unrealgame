"""Summarises Saved/asset_inventory.json (from inventory_assets.py) into Saved/asset_inventory_summary.json.
Run outside the editor: python Scripts/analyze_inventory.py"""
import collections, json, os
ROOT = os.path.join(os.path.dirname(__file__), '..', 'Saved')
R = json.load(open(os.path.join(ROOT, 'asset_inventory.json')))
BPP = {'DXT1': 0.5, 'BC4': 0.5, 'DXT3': 1, 'DXT5': 1, 'BC5': 1, 'BC7': 1, 'BC6H': 1, 'G8': 1, 'A8': 1, 'G16': 2, 'R16F': 2,
       'B8G8R8A8': 4, 'R8G8B8A8': 4, 'FloatRGBA': 8, 'A16B16G16R16': 8, 'A32B32G32R32F': 16, 'R32_FLOAT': 4}
for r in R:
    if r.get('dims') and 'x' in r['dims']:
        w, h = [int(x) for x in r['dims'].split('x')[:2]]
        f = (r.get('format') or '').replace('PF_', '')
        r['bpp'] = BPP.get(f)
        r['gpu_mb'] = round(w * h * (BPP.get(f) or 1) * 1.333 * (6 if r['class'] == 'TextureCube' else 1) / 1048576.0, 2)
        r['max_dim'] = max(w, h)
packs = collections.defaultdict(lambda: collections.Counter())
for r in R:
    p = packs[r['pack']]
    p['assets'] += 1
    p['disk_mb'] += r.get('disk_mb') or 0
    if r['class'] == 'StaticMesh':
        p['static_meshes'] += 1
        if r.get('nanite'):
            p['nanite_meshes'] += 1
            p['nanite_tris'] += r.get('nanite_tris') or 0
        else:
            p['plain_tris'] += r.get('tris') or 0
            if (r.get('tris') or 0) > 50000:
                p['heavy_plain_meshes'] += 1
    if r['class'] == 'SkeletalMesh':
        p['skeletal'] += 1
        p['nanite_tris'] += r.get('nanite_tris') or 0
    if 'gpu_mb' in r:
        p['textures'] += 1
        p['tex_gpu_mb'] += r['gpu_mb']
        if r.get('vt') == 'True':
            p['vt_textures'] += 1
        if r['max_dim'] >= 4096:
            p['tex_4k_plus'] += 1
        if (r.get('bpp') or 0) >= 4 or r.get('never_stream') == 'True':
            p['tex_problem'] += 1
summary = {'packs': {k: {kk: round(vv, 2) if isinstance(vv, float) else vv for kk, vv in v.items()} for k, v in packs.items()},
           'heavy_meshes': sorted([r for r in R if r['class'] in ('StaticMesh', 'SkeletalMesh')],
                                  key=lambda r: -((r.get('tris') or 0) if not r.get('nanite') else 0))[:40],
           'nanite_heavy': sorted([r for r in R if r.get('nanite')], key=lambda r: -(r.get('nanite_tris') or 0))[:25],
           'big_textures': sorted([r for r in R if 'gpu_mb' in r], key=lambda r: -r['gpu_mb'])[:30],
           'problem_textures': [r for r in R if 'gpu_mb' in r and ((r.get('bpp') or 0) >= 4 or r.get('never_stream') == 'True')],
           'totals': {'assets': len(R), 'disk_mb': round(sum(r.get('disk_mb') or 0 for r in R)),
                      'textures': sum(1 for r in R if 'gpu_mb' in r), 'tex_gpu_mb': round(sum(r.get('gpu_mb', 0) for r in R)),
                      'static_meshes': sum(1 for r in R if r['class'] == 'StaticMesh'),
                      'nanite_static': sum(1 for r in R if r['class'] == 'StaticMesh' and r.get('nanite')),
                      'skeletal': sum(1 for r in R if r['class'] == 'SkeletalMesh')}}
json.dump(summary, open(os.path.join(ROOT, 'asset_inventory_summary.json'), 'w'), indent=1)
print(json.dumps(summary['totals']))
for k, v in sorted(summary['packs'].items(), key=lambda x: -x[1]['disk_mb'])[:45]:
    print(f"{k[:52]:52s} disk {v['disk_mb']:8.0f}  sm {v.get('static_meshes',0):3d} nan {v.get('nanite_meshes',0):3d} plainTris {v.get('plain_tris',0)/1e3:8.0f}k heavy {v.get('heavy_plain_meshes',0):2d} sk {v.get('skeletal',0):3d} naniteTris {v.get('nanite_tris',0)/1e6:6.1f}M tex {v.get('textures',0):3d} texGPU {v.get('tex_gpu_mb',0):7.0f} 4k {v.get('tex_4k_plus',0):3d} vt {v.get('vt_textures',0):3d} prob {v.get('tex_problem',0)}")
