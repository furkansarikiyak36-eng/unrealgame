"""Cost comparison page for trees, grass, plants (bushes + ground plants) and rocks, per asset family.
Run outside the editor:  python Scripts/build_cost_compare_page.py
Inputs: Saved/asset_bundles.json, Saved/mesh_roles.json, Saved/mesh_sizes.json. Template: Scripts/cost_compare_template.html
Output: Saved/cost_compare.html (data embedded)
Family = a Megascans asset with its variants (VarA..), or one kit mesh.
- VRAM (family): every texture the variants use, counted once, + all variants' geometry (bundle model, caps applied).
- GPU, 100 close copies: non-Nanite LOD0 triangles x2 (shadow) x100 x 0.25 ms per million (RTX 3060 Laptop, High;
  calibrated on PerfTest). Nanite meshes draw by screen size, so no triangle figure is given for them.
- RAM, 1000 copies: instanced ~0.2 KB each (skeletal ~7 MB).
"""
import collections, json, os, re, statistics
ROOT = os.path.join(os.path.dirname(__file__), '..')
SAVED = os.path.join(ROOT, 'Saved')
B = {b['pkg']: b for b in json.load(open(os.path.join(SAVED, 'asset_bundles.json'), encoding='utf-8'))}
R = json.load(open(os.path.join(SAVED, 'mesh_roles.json'), encoding='utf-8'))
SIZE = json.load(open(os.path.join(SAVED, 'mesh_sizes.json')))
TAB = {'Ağaç': 'Ağaç', 'Çim': 'Çim', 'Çalı': 'Bitki', 'Zemin bitkisi': 'Bitki', 'Kaya / taş': 'Kaya'}
fams = collections.OrderedDict()
for p, r in R.items():
    tab = TAB.get(r['kind'])
    if not tab or p not in B:
        continue
    b = B[p]
    if p.startswith('/Game/Fab/Megascans/'):
        kit = 'Megascans'
        fam = re.sub(r'_[A-Za-z0-9]{5,10}$', '', r['family']).replace('_', ' ')
    else:
        kit = b['pack'].split('/')[0]
        fam = r['name']
    key = (tab, kit, fam)
    fams.setdefault(key, []).append((p, b, r))
rows = []
for (tab, kit, fam), items in fams.items():
    tex = {}
    geo = 0.0
    tris, gpu, nan, heights, roles, kinds, texmax = [], [], 0, [], collections.Counter(), collections.Counter(), 0
    for p, b, r in items:
        for t in b['tex_list']:
            tex[t['pkg']] = t['mb']
            try:
                texmax = max(texmax, max(int(x) for x in t['eff'].split('x')))
            except ValueError:
                pass
        geo += b['geo_mb'] or 0
        if b['nanite']:
            nan += 1
            tris.append(b['ntris'] or b['tris'])
        else:
            tris.append(b['tris'])
            gpu.append(b['tris'] * 2 * 100 * 0.25 / 1e6)
        s = SIZE.get(p) or [0, 0, 0]
        heights.append(s[2] / 100.0)
        roles[r['role']] += 1
        kinds[r['kind']] += 1
    vram = sum(tex.values()) + geo
    one = min((b['first_vram_mb'] for _, b, _ in items))
    rows.append({
        'tab': tab, 'kit': kit, 'fam': fam, 'kind': kinds.most_common(1)[0][0], 'role': roles.most_common(1)[0][0],
        'n': len(items), 'nanite': nan, 'tmin': min(tris), 'tmax': max(tris),
        'hmin': round(min(heights), 2), 'hmax': round(max(heights), 2),
        'vram': round(vram, 1), 'vone': round(one, 1), 'tex': len(tex), 'texmax': texmax,
        'gpu': round(statistics.median(gpu), 3) if gpu and nan == 0 else None,
        'gpumax': round(max(gpu), 3) if gpu and nan == 0 else None,
        'ram1000': round(len(items) and 0.2 * 1000 / 1024, 2),
        'meshes': [b['name'] for _, b, _ in items][:12],
    })
rows.sort(key=lambda x: (x['tab'], x['vram']))
summary = []
for tab in ('Ağaç', 'Çim', 'Bitki', 'Kaya'):
    for kit in sorted({r['kit'] for r in rows if r['tab'] == tab}):
        xs = [r for r in rows if r['tab'] == tab and r['kit'] == kit]
        g = [r['gpu'] for r in xs if r['gpu'] is not None]
        summary.append({'tab': tab, 'kit': kit, 'fams': len(xs), 'meshes': sum(r['n'] for r in xs),
                        'vram_med': round(statistics.median(r['vram'] for r in xs), 1),
                        'vram_max': max(r['vram'] for r in xs),
                        'tris_med': int(statistics.median((r['tmin'] + r['tmax']) / 2 for r in xs)),
                        'gpu_med': round(statistics.median(g), 3) if g else None,
                        'nanite': sum(r['nanite'] for r in xs)})
data = {'rows': rows, 'summary': summary}
tpl = open(os.path.join(ROOT, 'Scripts', 'cost_compare_template.html'), encoding='utf-8').read()
open(os.path.join(SAVED, 'cost_compare.html'), 'w', encoding='utf-8').write(
    tpl.replace('/*DATA*/null', json.dumps(data, ensure_ascii=False)))
print(len(rows), 'families')
for s in summary:
    print(s)
