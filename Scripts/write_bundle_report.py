"""Whole-asset ("bundle") cost of every placeable mesh: mesh + every part, material and texture it drags in + per-copy cost.

Inputs: Saved/asset_inventory.json, Saved/bundle_dependencies.json (bundle_dependencies.py), Saved/map_dependencies.json,
        Saved/asset_cost_per_mesh.csv (categories), Saved/cleanup_heavy.json + Saved/restore_texture_caps.json (texture caps)
Outputs: Saved/asset_bundles.json (also used by the table page), Saved/asset_bundles.csv, Docs/ASSET_PAKET_ANALIZI.md
Run outside the editor:  python Scripts/write_bundle_report.py

Model (estimates, explained in the report):
- textures: effective size after Max Texture Size caps, x pixel format, x1.333 for mips -> VRAM when fully streamed in
- geometry: non-Nanite = LOD0 vertex (36 B) + index (12 B per triangle) buffers x1.35 for the other LODs; Nanite = its
  data size on disk (streams into the 256 MB Nanite pool)
- shared: a texture used by more than one mesh in the project is paid once for all of them
- per copy: instanced (HISM/PCG/foliage) ~0.2 KB RAM in a game; Megaplant skeletal tree ~7 MB and per-frame bone update
"""
import collections
import csv
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
SAVED = os.path.join(HERE, '..', 'Saved')
INV = {r['package']: r for r in json.load(open(os.path.join(SAVED, 'asset_inventory.json')))}
B = json.load(open(os.path.join(SAVED, 'bundle_dependencies.json')))
RT, EO = B['runtime'], B['editor_only']
MAPS = json.load(open(os.path.join(SAVED, 'map_dependencies.json')))
MAIN, PT = set(MAPS['/Game/Maps/Main']), set(MAPS['/Game/Maps/PerfTest'])
CAT = {r['package']: r['category'] for r in csv.DictReader(open(os.path.join(SAVED, 'asset_cost_per_mesh.csv'), encoding='utf-8-sig'))}
CAPS = {}
try:
    for c in json.load(open(os.path.join(SAVED, 'cleanup_heavy.json')))['capped']:
        CAPS[c['texture']] = 2048
    for t, v in json.load(open(os.path.join(SAVED, 'restore_texture_caps.json'))):
        CAPS[t] = v
except FileNotFoundError:
    pass
# 2026-10-20: every KiteDemo / ParagonProps / SoulCave texture over 2048 was capped to 2048 and FloatRGBA over 1024 to
# 1024 (cap_kit_textures.py; the crashed first run left no full log), then single lists (texture_list_caps.jsonl).
KIT_ROOTS = ('/Game/KiteDemo/', '/Game/ParagonProps/', '/Game/SoulCave/')
for _p, _r in INV.items():
    if _p.startswith(KIT_ROOTS) and _r.get('class') in ('Texture2D', 'TextureCube'):
        CAPS.setdefault(_p, 1024 if (_r.get('format') or '') == 'FloatRGBA' else 2048)
for _log in ('kit_texture_caps.jsonl', 'texture_list_caps.jsonl'):
    try:
        for _line in open(os.path.join(SAVED, _log)):
            _row = json.loads(_line)
            CAPS[_row[0]] = _row[-1]
    except FileNotFoundError:
        pass
for _n in ('wooden_fence_albedo', 'wooden_fence_normal'):
    CAPS['/Game/UnrealExport/Textures/' + _n] = 1024
BPP = {'DXT1': 0.5, 'BC4': 0.5, 'DXT3': 1, 'DXT5': 1, 'BC5': 1, 'BC7': 1, 'BC6H': 1, 'G8': 1, 'A8': 1, 'G16': 2, 'R16F': 2,
       'B8G8R8A8': 4, 'R8G8B8A8': 4, 'FloatRGBA': 8, 'A16B16G16R16': 8, 'A32B32G32R32F': 16}
TEX_CLASSES = ('Texture2D', 'TextureCube', 'VirtualTexture2D', 'Texture2DArray')
MESH_CLASSES = ('StaticMesh', 'SkeletalMesh')
MAT_CLASSES = ('Material', 'MaterialInstanceConstant')


def tex_info(p):
    r = INV.get(p, {})
    dims = r.get('dims') or ''
    try:
        w, h = [int(x) for x in dims.split('x')[:2]]
    except ValueError:
        return None
    cap = CAPS.get(p) or 0
    ew, eh = w, h
    while cap and max(ew, eh) > cap:      # a cap drops whole mips
        ew, eh = max(1, ew // 2), max(1, eh // 2)
    fmt = r.get('format') or ''
    faces = 6 if r.get('class') == 'TextureCube' else 1
    mb = ew * eh * BPP.get(fmt, 1) * 1.333 * faces / 1048576.0
    return {'pkg': p, 'name': r.get('asset'), 'dims': dims, 'eff': '{}x{}'.format(ew, eh), 'fmt': fmt, 'mb': round(mb, 2),
            'capped': bool(cap and cap < max(w, h)), 'disk': round(r.get('disk_mb') or 0, 2)}


def geo_mb(r):
    if r.get('nanite') or (r.get('nanite_tris') or 0) > 0:
        return r.get('disk_mb') or 0
    tris, verts = r.get('tris') or 0, r.get('verts') or 0
    return (verts * 36 + tris * 12) * (1.35 if (r.get('lods') or 1) > 1 else 1.0) / 1048576.0


# how many meshes use each texture (sharing)
users = collections.Counter()
for m, deps in RT.items():
    for d in set(deps):
        if INV.get(d, {}).get('class') in TEX_CLASSES:
            users[d] += 1

bundles = []
for m, deps in RT.items():
    r = INV.get(m)
    if not r:
        continue
    tex = [t for t in (tex_info(d) for d in deps if INV.get(d, {}).get('class') in TEX_CLASSES) if t]
    for t in tex:
        t['users'] = users[t['pkg']]
    parts = [d for d in deps if INV.get(d, {}).get('class') in MESH_CLASSES]
    mats = [d for d in deps if INV.get(d, {}).get('class') in MAT_CLASSES]
    other = collections.Counter(INV[d]['class'] for d in deps if d in INV and INV[d]['class'] not in TEX_CLASSES + MESH_CLASSES + MAT_CLASSES)
    skeletal = r['class'] == 'SkeletalMesh'
    nanite = bool(r.get('nanite') or (r.get('nanite_tris') or 0) > 0)
    geo = geo_mb(r) + sum(geo_mb(INV[p]) for p in parts)
    tex_mb = sum(t['mb'] for t in tex)
    own_tex = sum(t['mb'] for t in tex if t['users'] <= 1)
    disk = (r.get('disk_mb') or 0) + sum(INV[d].get('disk_mb') or 0 for d in deps if d in INV)
    eo_disk = 0.0
    for e in EO.get(m, []):
        eo_disk += INV.get(e, {}).get('disk_mb') or 0
    tris = int(r.get('tris') or 0)
    per_ram_kb = 7 * 1024 if skeletal else 0.2
    per_gpu = 'Nanite' if nanite else '{:.1f}k üçgen'.format(tris / 1000)
    bundles.append({
        'pkg': m, 'name': r['asset'], 'pack': r['pack'], 'cat': CAT.get(m, ''), 'class': 'Skeletal' if skeletal else 'Statik',
        'nanite': nanite, 'tris': tris, 'ntris': int(r.get('nanite_tris') or 0), 'lods': r.get('lods'), 'bones': r.get('bones'),
        'parts': len(parts), 'materials': len(mats), 'textures': len(tex), 'other': dict(other),
        'geo_mb': round(geo, 2), 'tex_mb': round(tex_mb, 1), 'own_tex_mb': round(own_tex, 1), 'shared_tex_mb': round(tex_mb - own_tex, 1),
        'first_vram_mb': round(geo + tex_mb, 1), 'disk_mb': round(disk, 1), 'editor_only_mb': round(eo_disk, 1),
        'per_copy_ram_kb': per_ram_kb, 'per_copy_gpu': per_gpu,
        'ram_100_mb': round(per_ram_kb * 100 / 1024, 2), 'ram_1000_mb': round(per_ram_kb * 1000 / 1024, 1),
        'gpu_close_tris_x2': tris * 2 if not nanite else None,
        'used': ('Main ' if m in MAIN else '') + ('PerfTest' if m in PT else ''),
        'tex_list': sorted(tex, key=lambda t: -t['mb']), 'mat_list': [INV[x]['asset'] for x in mats], 'part_list': [INV[x]['asset'] for x in parts]})

json.dump(bundles, open(os.path.join(SAVED, 'asset_bundles.json'), 'w'))
keys = ['name', 'pack', 'cat', 'class', 'nanite', 'tris', 'ntris', 'lods', 'bones', 'parts', 'materials', 'textures', 'geo_mb', 'tex_mb',
        'own_tex_mb', 'shared_tex_mb', 'first_vram_mb', 'disk_mb', 'editor_only_mb', 'per_copy_ram_kb', 'per_copy_gpu', 'ram_100_mb',
        'ram_1000_mb', 'used', 'pkg']
with open(os.path.join(SAVED, 'asset_bundles.csv'), 'w', newline='', encoding='utf-8-sig') as f:
    w = csv.DictWriter(f, fieldnames=keys, extrasaction='ignore')
    w.writeheader()
    w.writerows(sorted(bundles, key=lambda b: (b['pack'], b['name'])))

# ---------------------------------------------------------------- report
by_name = {}
for b in bundles:
    by_name.setdefault(b['name'], b)
EXAMPLES = [  # (title, mesh name) — one per kind of thing the game uses
    ('Ağaç — Megaplant European Aspen 01 B', 'Tree_European_Aspen_01_B'),
    ('Ağaç — Megaplant Dead Pine D', 'Tree_Dead_Pine_D'),
    ('Ağaç — ProceduralNtr_vol2 çam', 'pine_tree_full_a'),
    ('Ağaç — KiteDemo Scots Pine', 'ScotsPine_01'),
    ('Ağaç/çalı — PN coniferBushes Larch 07', 'Larch_07'),
    ('Kütük — UnrealExport 600 cm', 'SM_RawLog_600'),
    ('Kaya — Megascans Mossy Forest Boulder', 'SM_wfstcdnaw'),
    ('Çalı — Megascans European Spindle VarC', 'SM_wk1ncbxja_VarC'),
    ('Çalı — Megascans Elderberry VarA', 'SM_wfzobb2ia_VarA'),
    ('Çim — Megascans Kikuyu VarA', 'SM_wiljearqx_VarA'),
    ('Çiçek — PN MeadowFlowers', 'flower_20_03'),
]


def find(name):
    if name in by_name:
        return by_name[name]
    for b in bundles:
        if name.lower() in b['pkg'].lower():
            return b
    return None


def fmt(x, d=1):
    return ('{:,.' + str(d) + 'f}').format(x).replace(',', ' ').replace('.', ',') if x is not None else '—'


L = []
A = L.append
A('# Asset paket analizi — mesh + materyal + doku + kopya maliyeti (2026-10-19)\n')
A('Her yerleştirilebilir mesh için, sahneye konduğunda peşinden gelen her şey: kendi geometrisi, bağlı parçaları, materyalleri, '
  'dokuları (bugünkü 2K sınırları dahil), iskelet/fizik asset\'i; üstüne her kopyanın maliyeti. {} mesh. '
  'Tam liste: `Saved/asset_bundles.csv` (Excel), tablo sayfasında satıra tıklayınca da açılıyor.\n'.format(len(bundles)))
A('## Nasıl okunur\n')
A('| Kavram | Anlamı |')
A('|---|---|')
A('| **İlk kopya VRAM** | Asset ilk kez ekranda tam yüklenince: geometri + tüm dokuları. Sonraki kopyalar bunu tekrar ödemez. |')
A('| **Kendi dokusu / paylaşılan doku** | Paylaşılan doku başka mesh\'lerle ortak; aynı paketten ikinci bir asset kullanınca tekrar ödenmez. |')
A('| **Kopya başı RAM** | Instanced (HISM/PCG/foliage) ~0,2 KB; Megaplant skeletal ağaç ~7 MB (bugün 500 ağaçla ölçüldü). |')
A('| **Kopya başı GPU** | Nanite\'sız: yakında en detaylı halin üçgeni, gölge açıksa iki kez çizilir; uzaklaştıkça LOD\'lar düşer. Nanite: ekrana göre, kopya sayısından az etkilenir. |')
A('| **Disk** | Asset + bütün bağımlılıkları; Megaplant\'ın üretim verisi ("yalnız editör") ayrı yazıldı, oyuna girmez. |\n')
A('Sahneden bağımsız sabit maliyet (her sahnede): VRAM ~1,4 GB motor havuzları, GPU 2,5 ms boş harita, 6,6 ms gökyüzü+ışık seti (High).\n')
A('## 1. Örnek kartlar\n')
for title, name in EXAMPLES:
    b = find(name)
    if not b:
        continue
    A('### {}\n'.format(title))
    A('`{}` — paket {}, {}{}, kullanan: {}\n'.format(b['name'], b['pack'], b['class'], ' (Nanite)' if b['nanite'] else '', b['used'] or 'hiçbir harita'))
    A('| | |')
    A('|---|---|')
    if b['nanite'] and b['class'] == 'Skeletal':
        A('| Geometri | {} M Nanite üçgen, {} kemik, Nanite verisi ~{} MB |'.format(fmt(b['ntris'] / 1e6), int(b['bones'] or 0), fmt(b['geo_mb'])))
    elif b['nanite']:
        A('| Geometri | Nanite, verisi ~{} MB |'.format(fmt(b['geo_mb'])))
    else:
        A('| Geometri | LOD0 {} üçgen, {} LOD, tampon ~{} MB |'.format(fmt(b['tris'], 0), int(b['lods'] or 0), fmt(b['geo_mb'], 2)))
    A('| Materyal | {} ({}) |'.format(b['materials'], ', '.join(b['mat_list'][:4]) + (' …' if len(b['mat_list']) > 4 else '')))
    A('| Doku | {} adet, {} MB (kendi {} / paylaşılan {}) |'.format(b['textures'], fmt(b['tex_mb']), fmt(b['own_tex_mb']), fmt(b['shared_tex_mb'])))
    if b['parts']:
        A('| Parça mesh | {} |'.format(b['parts']))
    if b['other']:
        A('| Diğer | {} |'.format(', '.join('{} {}'.format(v, k) for k, v in b['other'].items())))
    A('| **İlk kopya VRAM** | **~{} MB** |'.format(fmt(b['first_vram_mb'])))
    A('| Disk | {} MB{} |'.format(fmt(b['disk_mb']), ' (+ {} MB yalnız editör üretim verisi)'.format(fmt(b['editor_only_mb'])) if b['editor_only_mb'] else ''))
    A('| Kopya başı | RAM {}, GPU {} |'.format('~7 MB' if b['class'] == 'Skeletal' else '~0,2 KB', b['per_copy_gpu'] + ('' if b['nanite'] else ' (+gölge ×2)')))
    A('| 100 / 1.000 kopya | RAM ~{} MB / ~{} MB{} |'.format(fmt(b['ram_100_mb'], 2), fmt(b['ram_1000_mb']),
                                                             '' if b['nanite'] else '; yakında görünen her kopya {} üçgen'.format(fmt(b['tris'] * 2, 0))))
    A('')
    A('Dokular: ' + '; '.join('{} {}→{} {} {} MB{}'.format(t['name'], t['dims'], t['eff'], t['fmt'], fmt(t['mb']), ' (paylaşılan)' if t['users'] > 1 else '')
                            for t in b['tex_list'][:8]) + (' …' if len(b['tex_list']) > 8 else '') + '\n')

A('## 2. Türe göre: ilk kopya VRAM ve kopya başı maliyet\n')
A('Her türde en ucuzdan pahalıya; "ilk kopya" asset\'i sahneye almanın bedeli, "kopya başı" her ek kopyanın. Binlerce kopyalanacak türlerde '
  '(çim, çalı, taş) kopya başı GPU belirleyici; birkaç kopya konacaklarda (ağaç, yapı) ilk kopya VRAM\'i.\n')
order = ['Ağaç', 'Çalı', 'Çim / çiçek / zemin bitkisi', 'Kaya / taş', 'Kütük / odun', 'Yapı / eşya / diğer']
for cat in order:
    bs = [b for b in bundles if b['cat'] == cat and b['name'] != 'SK_Mannequin']
    if not bs:
        continue
    A('### {} ({} asset)\n'.format(cat, len(bs)))
    A('| Paket | Asset | İlk kopya VRAM MB (min–medyan–max) | Kopya başı GPU (medyan) | Nanite | Disk MB (medyan) |')
    A('|---|---|---|---|---|---|')
    byp = collections.defaultdict(list)
    for b in bs:
        byp[b['pack']].append(b)
    rows = []
    for k, xs in byp.items():
        v = sorted(x['first_vram_mb'] for x in xs)
        t = sorted(x['tris'] for x in xs if not x['nanite'])
        rows.append((v[len(v) // 2], k, xs, v, t))
    for med, k, xs, v, t in sorted(rows):
        nan = sum(x['nanite'] for x in xs)
        A('| {} | {} | {} – {} – {} | {} | {}/{} | {} |'.format(k, len(xs), fmt(v[0]), fmt(med), fmt(v[-1]),
            ('{} üçgen'.format(fmt(t[len(t) // 2], 0)) if t else 'Nanite'), nan, len(xs), fmt(sorted(x['disk_mb'] for x in xs)[len(xs) // 2])))
    A('')

A('## 3. En pahalı 20 asset (ilk kopya VRAM)\n')
A('| Asset | Paket | Tür | İlk kopya VRAM MB | Doku MB (kendi) | Geometri MB | Kopya başı GPU | Kullanan |')
A('|---|---|---|---|---|---|---|---|')
for b in sorted(bundles, key=lambda b: -b['first_vram_mb'])[:20]:
    A('| {} | {} | {} | {} | {} ({}) | {} | {} | {} |'.format(b['name'], b['pack'], b['cat'], fmt(b['first_vram_mb']), fmt(b['tex_mb']),
                                                              fmt(b['own_tex_mb']), fmt(b['geo_mb'], 1), b['per_copy_gpu'], b['used'] or '—'))
A('\n## 4. Kopya başına en pahalı 20 asset (yakında, Nanite\'sız)\n')
A('| Asset | Paket | Tür | LOD0 üçgen | Gölgeyle | 100 kopya yakında | İlk kopya VRAM MB | Kullanan |')
A('|---|---|---|---|---|---|---|---|')
for b in sorted([b for b in bundles if not b['nanite']], key=lambda b: -b['tris'])[:20]:
    A('| {} | {} | {} | {} | {} | {} M üçgen | {} | {} |'.format(b['name'], b['pack'], b['cat'], fmt(b['tris'], 0), fmt(b['tris'] * 2, 0),
                                                           fmt(b['tris'] * 200 / 1e6), fmt(b['first_vram_mb']), b['used'] or '—'))
open(os.path.join(HERE, '..', 'Docs', 'ASSET_PAKET_ANALIZI.md'), 'w', encoding='utf-8').write('\n'.join(L) + '\n')
print('bundles', len(bundles), 'lines', len(L))
