"""Per-mesh cost model and the full asset report: Docs/ASSET_MALIYET_RAPORU.md and Saved/asset_cost_per_mesh.csv.

Inputs (from the editor scripts): Saved/asset_inventory.json (inventory_assets.py), Saved/mesh_dependencies.json
(mesh_dependencies.py), Saved/map_dependencies.json (map_dependencies.py). Run outside the editor:
    python Scripts/write_asset_cost_report.py

Cost model (estimates; see the report's method section):
- mesh_mb: non-Nanite vertex + index buffers of LOD0 (36 B per vertex, 4 B per index) x 1.35 for the other LODs.
  Nanite meshes: the .uasset/.ubulk size on disk (the Nanite data streams from it into the 256 MB pool).
- tex_mb: the mesh's textures (through its materials) at full resolution with mips, by pixel format.
- per instance, GPU: triangles drawn when close (non-Nanite LOD0, x2 with shadows); Nanite draws by screen size.
- per instance, RAM: instanced (HISM/PCG/foliage) ~0.2 KB in a game, ~1 KB in the editor (measured 0.7-1.1 GB per
  million); a skeletal tree actor ~7 MB (measured: 500 Megaplant trees +3.5 GB).
"""
import collections
import csv
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
SAVED = os.path.join(HERE, '..', 'Saved')
INV = json.load(open(os.path.join(SAVED, 'asset_inventory.json')))
DEPS = json.load(open(os.path.join(SAVED, 'mesh_dependencies.json')))
MAPS = json.load(open(os.path.join(SAVED, 'map_dependencies.json')))
MAIN, PT = set(MAPS['/Game/Maps/Main']), set(MAPS['/Game/Maps/PerfTest'])
BPP = {'DXT1': 0.5, 'BC4': 0.5, 'DXT3': 1, 'DXT5': 1, 'BC5': 1, 'BC7': 1, 'BC6H': 1, 'G8': 1, 'A8': 1, 'G16': 2, 'R16F': 2,
       'B8G8R8A8': 4, 'R8G8B8A8': 4, 'FloatRGBA': 8, 'A16B16G16R16': 8, 'A32B32G32R32F': 16}
BY_PKG = {r['package']: r for r in INV}

for r in INV:
    if r.get('dims') and 'x' in r['dims']:
        try:
            w, h = [int(x) for x in r['dims'].split('x')[:2]]
        except ValueError:
            continue
        r['w'], r['h'] = w, h
        r['gpu_mb'] = w * h * BPP.get(r.get('format') or '', 1) * 1.333 * (6 if r['class'] == 'TextureCube' else 1) / 1048576.0

CATS = [  # (category, keywords in the lower-case package path); first match wins
    ('Karakter', ['mannequin', '/character', 'sk_']),
    ('Kütük / odun', ['twig', 'branch', '_log', 'log_', '/log', 'stump', 'firewood', 'beam', 'plank', 'stick', 'debris', 'wooden']),
    ('Kaya / taş', ['rock', 'stone', 'boulder', 'cliff', 'scree', 'pebble', 'cobble']),
    ('Çalı', ['bush', 'shrub', 'spindle', 'elderberry', 'raspberry', 'blackberry', 'blueberry', 'locust', 'hydrangea', 'myrtle', 'heather',
              'wk1ncbxja', 'wfzobb2ia', 'wf0oefeja', 'wdvhberja', 'vgztealha', 'vdknafgha']),
    ('Ağaç', ['tree', 'pine', 'larch', 'spruce', 'birch', 'oak', 'aspen', 'cypress', 'maple', 'rowan', 'hilltree']),
    ('Çim / çiçek / zemin bitkisi', ['grass', 'flower', 'fern', 'clover', 'weed', 'plant', 'nettle', 'sorrel', 'ivy', 'poppy', 'meadow',
                                    'burdock', 'sage', 'balsam', 'archangel', 'pratia', 'lily', 'ground_cover', 'cordyline', 'palm',
                                    'reed', 'buttercup', 'scabious', 'yarrow', 'leaves', 'marshcress', 'leaf']),
]


def category(r):
    p = r['package'].lower()
    if r['class'] == 'SkeletalMesh' and 'megaplant' in p:
        return 'Ağaç'
    if r['pack'] == 'PN_coniferBushes_I' and 'twig' not in p and 'mannequin' not in p:
        return 'Çalı'           # young pines, spruces and larches sold as conifer bushes
    for name, keys in CATS:
        if any(k in p for k in keys):
            return name
    return 'Yapı / eşya / diğer'


def used(p):
    return ('Main ' if p in MAIN else '') + ('PerfTest' if p in PT else '')


def quality(r, tex_max):
    """Short quality note: texture resolution, LODs, Nanite."""
    if tex_max >= 4096:
        t = '4K+ doku'
    elif tex_max >= 2048:
        t = '2K doku'
    elif tex_max >= 1024:
        t = '1K doku'
    elif tex_max:
        t = '≤512 doku'
    else:
        t = 'doku yok'
    g = 'Nanite' if r.get('nanite') else '{} LOD'.format(int(r['lods'])) if r.get('lods') else 'LOD ?'
    return t + ', ' + g


rows = []
for r in INV:
    if r['class'] not in ('StaticMesh', 'SkeletalMesh'):
        continue
    dep = DEPS.get(r['package'], {'materials': [], 'textures': []})
    tex = [BY_PKG[t] for t in dep['textures'] if t in BY_PKG and 'gpu_mb' in BY_PKG[t]]
    tex_mb = sum(t['gpu_mb'] for t in tex)
    tex_max = max([max(t['w'], t['h']) for t in tex] or [0])
    tris = r.get('tris') or 0
    verts = r.get('verts') or 0
    nanite = bool(r.get('nanite'))
    skeletal = r['class'] == 'SkeletalMesh'
    if nanite or (skeletal and (r.get('nanite_tris') or 0) > 0):
        mesh_mb = r['disk_mb']
        gpu_close = 'Nanite (ekrana göre)'
        tris_close = None
    else:
        mesh_mb = (verts * 36 + tris * 12) * (1.35 if (r.get('lods') or 1) > 1 else 1.0) / 1048576.0
        gpu_close = '{:.0f}k (+gölge ×2)'.format(tris / 1000)
        tris_close = tris
    rows.append({
        'mesh': r['asset'], 'pack': r['pack'], 'category': category(r), 'class': 'Skeletal' if skeletal else 'Static',
        'nanite': 'evet' if (nanite or (skeletal and (r.get('nanite_tris') or 0) > 0)) else 'hayır',
        'tris_lod0': int(tris), 'nanite_tris': int(r.get('nanite_tris') or 0), 'lods': int(r['lods']) if r.get('lods') else None,
        'bones': int(r['bones']) if r.get('bones') else None,
        'materials': len(dep['materials']), 'textures': len(tex), 'tex_max': tex_max,
        'mesh_mb': round(mesh_mb, 2), 'tex_mb': round(tex_mb, 1), 'disk_mb': round(r['disk_mb'], 2),
        'gpu_close': gpu_close, 'tris_close': tris_close,
        'ram_per_instance': '~7 MB (actor)' if skeletal else '~0,2 KB oyun / ~1 KB editör (instanced)',
        'quality': quality(r, tex_max), 'used_in': used(r['package']).strip() or '—', 'package': r['package'],
    })

with open(os.path.join(SAVED, 'asset_cost_per_mesh.csv'), 'w', newline='', encoding='utf-8-sig') as f:
    w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
    w.writeheader()
    w.writerows(sorted(rows, key=lambda x: (x['pack'], x['category'], x['mesh'])))

# ---- pack level (textures de-duplicated per pack)
packs = collections.OrderedDict()
for r in INV:
    p = packs.setdefault(r['pack'], {'assets': 0, 'disk': 0.0, 'meshes': 0, 'nanite': 0, 'skeletal': 0, 'tex': 0, 'tex_mb': 0.0,
                                     'tex4k': 0, 'plain_tris': 0, 'heavy': 0, 'main': 0, 'cats': collections.Counter()})
    p['assets'] += 1
    p['disk'] += r['disk_mb']
    p['main'] += r['package'] in MAIN
    if 'gpu_mb' in r:
        p['tex'] += 1
        p['tex_mb'] += r['gpu_mb']
        p['tex4k'] += max(r['w'], r['h']) >= 4096
for x in rows:
    p = packs[x['pack']]
    p['meshes'] += 1
    p['nanite'] += x['nanite'] == 'evet'
    p['skeletal'] += x['class'] == 'Skeletal'
    p['cats'][x['category']] += 1
    if x['tris_close']:
        p['plain_tris'] += x['tris_close']
        p['heavy'] += x['tris_close'] > 50000

NEW = {'SoulCave', 'KiteDemo', 'PN_MeadowFlowers', 'PN_Fern', 'PN_coniferBushes_I', 'PN_WildBerries', 'PN_tropicalGroundPlants'}


def pack_name(k):
    return k + (' 🆕' if k in NEW else '')


def verdict(p):
    if p.get('n', 1) > 1:
        return 'Hafif' if not p['heavy'] else 'Orta'
    if p['tex_mb'] > 1000 or p['heavy'] >= 3 or p['disk'] > 1500:
        return 'Ağır'
    if p['tex_mb'] > 200 or p['heavy'] or p['disk'] > 300:
        return 'Orta'
    return 'Hafif'


L = []
A = L.append
A('# Content Drawer — asset maliyet raporu (2026-10-19)\n')
A('Bütün `/Game` içeriği: {} asset, {:.1f} GB disk. Bunun {} mesh\'i (statik + skeletal) tek tek hesaplandı; tam liste '
  '`Saved/asset_cost_per_mesh.csv` (Excel\'de açılır, her satır bir mesh).\n'.format(len(INV), sum(r['disk_mb'] for r in INV) / 1024, len(rows)))
A('Yeni eklenen kitler 🆕 ile işaretli: SoulCave, KiteDemo, PN_MeadowFlowers, PN_Fern, PN_coniferBushes_I, PN_WildBerries, PN_tropicalGroundPlants.\n')
A('## Nasıl okunur (ölçüm ve tahmin yöntemi)\n')
A('| Sütun | Ne demek | Nereden |')
A('|---|---|---|')
A('| LOD0 üçgen | Yakından çizilen en detaylı hal | Asset etiketi (kesin) |')
A('| Nanite | Evetse üçgen sayısı çizimi az etkiler, ekrana göre çizilir | Asset etiketi (kesin) |')
A('| Mesh MB | Nanite\'sız: vertex+index tamponu (LOD0 × 1,35). Nanite: diskteki veri (256 MB akış havuzundan akar) | Tahmin |')
A('| Doku MB | Mesh\'in materyallerinin dokuları, tam çözünürlük + mip\'ler. Oyunda akıtılır; ilk kez görünürken bu kadar VRAM ister, aynı dokuyu paylaşan mesh\'ler bir kez öder | Tahmin |')
A('| Kopya başı GPU | Yakındaki her kopya: Nanite\'sızda LOD0 üçgeni (gölgeyle 2 kez çizilir); uzakta LOD\'lar düşer | Kesin + model |')
A('| Kopya başı RAM | Instanced (HISM/PCG/foliage) ~0,2 KB oyunda, ~1 KB editörde; skeletal ağaç (Megaplant) ~7 MB | Bugünkü ölçümler |')
A('| Kalite | Doku çözünürlüğü + LOD/Nanite | Asset etiketi |\n')
A('Önemli: bir asset yalnız bir haritada kullanılırsa oyunda maliyet yaratır. "Kullanan" sütunu Main ve PerfTest\'e göre.\n')
A('Sabit motor maliyeti (her sahnede, asset\'ten bağımsız): VRAM ~1,4 GB (gölge, Lumen, Nanite, distance field havuzları), '
  'GPU 2,5 ms boş haritada, 6,6 ms gökyüzü+ışık setiyle (High kalite).\n')
A('## 1. Paketler\n')
A('| Paket | Karar | Disk MB | Mesh (Nanite / skeletal) | Ne var | Doku (4K+) | Doku MB | Nanite\'sız LOD0 toplam | >50k mesh | Main\'de kullanılan |')
A('|---|---|---|---|---|---|---|---|---|---|')
groups = collections.OrderedDict()
for k, p in packs.items():
    key = k
    if k.startswith('Fab/Megascans/') and p['disk'] < 50 and not p['heavy']:
        key = 'Fab/Megascans/' + k.split('/')[2] + ' (küçük paketler)'
    g = groups.setdefault(key, {'assets': 0, 'disk': 0.0, 'meshes': 0, 'nanite': 0, 'skeletal': 0, 'tex': 0, 'tex_mb': 0.0,
                                'tex4k': 0, 'plain_tris': 0, 'heavy': 0, 'main': 0, 'cats': collections.Counter(), 'n': 0})
    for kk in ('assets', 'disk', 'meshes', 'nanite', 'skeletal', 'tex', 'tex_mb', 'tex4k', 'plain_tris', 'heavy', 'main'):
        g[kk] += p[kk]
    g['cats'].update(p['cats'])
    g['n'] += 1
for k, p in sorted(groups.items(), key=lambda x: -x[1]['disk']):
    if p['disk'] < 5 and not p['heavy']:
        continue
    if p.get('n', 1) > 1:
        k = '{} ×{}'.format(k, p['n'])
    what = ', '.join('{} {}'.format(n, c) for c, n in p['cats'].most_common(3)) if p['cats'] else '—'
    A('| {} | {} | {:.0f} | {} ({} / {}) | {} | {} ({}) | {:.0f} | {:.0f}k | {} | {} / {} |'.format(
        pack_name(k), verdict(p), p['disk'], p['meshes'], p['nanite'], p['skeletal'], what, p['tex'], p['tex4k'], p['tex_mb'],
        p['plain_tris'] / 1000, p['heavy'], p['main'], p['assets']))
A('\n5 MB\'tan küçük ve ağır mesh\'i olmayan paketler (çoğu tek tek Megascans asset\'i) tabloda yok; hepsi hafif.\n')

A('## 2. Türe göre seçenekler\n')
A('Her türde: kopya başı en ucuz ve dokusu en hafif olanlar önce. "Kopya başı GPU" yakından çizilen üçgen; '
  'binlerce kopyalanacak şeylerde (çim, çalı) bu ve dokular belirleyici, birkaç tane konacak şeylerde (ağaç, yapı) daha az.\n')
order = ['Ağaç', 'Çalı', 'Çim / çiçek / zemin bitkisi', 'Kaya / taş', 'Kütük / odun', 'Yapı / eşya / diğer', 'Karakter']
for cat in order:
    cr = [x for x in rows if x['category'] == cat]
    if not cr:
        continue
    A('### {} ({} mesh)\n'.format(cat, len(cr)))
    bypack = collections.defaultdict(list)
    for x in cr:
        bypack[x['pack']].append(x)
    A('| Paket | Mesh | Nanite | LOD0 üçgen (min-medyan-max) | Doku MB / mesh (medyan) | En büyük doku | Kalite (medyan) |')
    A('|---|---|---|---|---|---|---|')
    for k, xs in sorted(bypack.items(), key=lambda kv: -len(kv[1])):
        t = sorted(x['tris_lod0'] for x in xs)
        tm = sorted(x['tex_mb'] for x in xs)
        nan = sum(x['nanite'] == 'evet' for x in xs)
        A('| {} | {} | {}/{} | {:.1f}k – {:.1f}k – {:.1f}k | {:.0f} | {} | {} |'.format(
            pack_name(k), len(xs), nan, len(xs), t[0] / 1000, t[len(t) // 2] / 1000, t[-1] / 1000, tm[len(tm) // 2],
            max(x['tex_max'] for x in xs) or '—', xs[len(xs) // 2]['quality']))
    A('')
    A('En ağır 8 (yakından, Nanite\'sız):\n')
    A('| Mesh | Paket | LOD0 üçgen | LOD | Doku MB | Kullanan |')
    A('|---|---|---|---|---|---|')
    for x in sorted([x for x in cr if x['tris_close']], key=lambda x: -x['tris_close'])[:8]:
        A('| {} | {} | {:.0f}k | {} | {:.0f} | {} |'.format(x['mesh'], pack_name(x['pack']), x['tris_close'] / 1000, x['lods'], x['tex_mb'], x['used_in']))
    A('')

A('## 3. Kopya başına maliyet — hızlı karşılaştırma\n')
A('| Ne | Örnek | Kopya başı RAM | Yakından GPU | 1.000 kopya | Not |')
A('|---|---|---|---|---|---|')
A('| Instanced bitki (Nanite\'sız) | Kikuyu çim 1,4-5,3k üçgen | ~0,2 KB (editörde ~1 KB) | 1,4-5,3k üçgen (+gölge) | RAM ~0,2 MB, GPU yakındakilere bağlı | Bugünkü ölçüm: 7.500 çim +4,8 ms (gölge+Lumen dahil) |')
A('| Instanced çalı (Nanite\'sız) | Spindle VarC 154k | ~0,2 KB | 154k üçgen (+gölge) | Yakında onlarcası bile ağır | Nanite açılmalı |')
A('| Instanced Nanite taş/kaya | Megascans 3D | ~0,2 KB | Ekrana göre, ucuz | Çok ucuz | En verimli tür |')
A('| Skeletal ağaç (Megaplant) | Aspen 6 M Nanite | ~7 MB | Nanite, ucuz | RAM ~7 GB, CPU +5 ms | 500 ağaç ölçüldü: RAM +3,5 GB, oyun thread +2,5 ms |')
A('| Statik ağaç (PNP / KiteDemo) | Nanite açılırsa | ~0,2 KB | Nanite ile ucuz | Ucuz | Orman için doğru seçim |')
A('| Ayrı actor olarak yerleştirilmiş mesh | Bina, eşya | ~1-2 KB + bileşen | Mesh\'e bağlı | Onlarca-yüzlerce için uygun | Binlerce için instanced kullan |\n')

A('## 4. En pahalı dokular ve sorunlu formatlar\n')
A('| Doku | Boyut | Format | Tahmini MB | Paket | Kullanan |')
A('|---|---|---|---|---|---|')
for r in sorted([r for r in INV if 'gpu_mb' in r], key=lambda r: -r['gpu_mb'])[:25]:
    A('| {} | {} | {} | {:.0f} | {} | {} |'.format(r['asset'], r['dims'], r.get('format') or '?', r['gpu_mb'], pack_name(r['pack']), used(r['package']) or '—'))
A('\nFloatRGBA (8 B/piksel) ve B8G8R8A8 (4 B/piksel) sıkıştırılmamış; DXT1 0,5, DXT5/BC5/BC7 1 B/piksel.\n')
A('## 5. Sonuç ve öneriler\n')
A('**Geometri olarak en hafif:** Megascans 3D (taş, kaya, kütük; hepsi Nanite), UnrealExport kütükleri (58 mesh ~12k üçgen), '
  "PN_coniferBushes_I genç kozalaklılar (28-3,5k üçgen, 4 LOD), PN_WildBerries çalıları (0,1-7k), PN_MeadowFlowers, Megascans çim/çiçekleri.")
A("")
A('**Doku olarak en ağır (VRAM ve disk):** KiteDemo (5,1 GB, 92 adet 4K; ağaç başına 100-190 MB doku), ProceduralNtr_vol2 (4,2 GB), '
  "OldWest (2,0 GB; Main'de kullanılıyor), B3D (1,7 GB, çoğu kullanılmayan 8K), ProceduralNaturePack (1,1 GB), SoulCave (0,66 GB), PN_Fern (0,52 GB). "
  "Pure Polygons kitlerinin dokuları da 4K (mesh başına 40-85 MB).")
A("")
A("**Kopya başı GPU olarak en riskli (Nanite'sız, yakında çok üçgen):** ProceduralNtr_vol2 çamları (~260k, Main'de kullanılıyor), "
  "large_tree_w_roots_a (132k, Main), Spindle VarC/VarD (154k/82k), Black Locust VarC (71k), KiteDemo ağaçları (28-60k).")
A("")
A("**Ağaç seçimi için:**")
A("- **PN_coniferBushes_I** (yeni): en hafif geometri, 4 LOD, statik, instanced olabilir. Boyları ve görünüşü ağaç olarak yetiyorsa orman için en ucuz seçenek. Dokuları 2K'ya indirmek VRAM'i dörtte birine düşürür.")
A("- **KiteDemo ağaçları** (ScotsPine, HillTree): Epic'in açık dünya demosu; 28-60k üçgen, Nanite kapalı, dokular çok büyük. Nanite açılıp dokular 2K'ya inerse kaliteli bir orta yol.")
A("- **ProceduralNtr_vol2 çamları:** 260k üçgen, Nanite kapalı, 250-300 MB doku. Main'de kullanılıyor; Nanite açılmadan ormanda kullanılmamalı. Uzak kart versiyonları (pine_tree_card_distant, 8-60 üçgen) uzak orman için çok ucuz.")
A("- **SoulCave ağaçları:** mobil demo, düşük poligon ve 1-2K doku; stil olarak gerçekçi Megascans'la uyuşmayabilir.")
A("- **Megaplant:** GPU'da ucuz (Nanite), ama skeletal: ağaç başına ~7 MB RAM ve CPU. Az sayıda kesilebilir ağaç için.")
A("")
A('**Kullanılmayan ve silinirse yer açanlar** (yalnız disk/editör; onay gerekir): ProceduralNtr_vol2 HDRI 1,6 GB + zemin dokusu 682 MB, '
  "ProceduralNaturePack HDRI 207 MB, B3D 8K RAW setleri ~1,2 GB, Japanese Cypress 01_A. KiteDemo 6,5 GB tutuyor ve henüz hiçbir haritada yok.")
A("")
A("**Toplu ve geri alınabilir iyileştirmeler:** 4K dokuları 2K'ya sınırlamak (Max Texture Size; asset silinmez), ağır Nanite'sız mesh'lere Nanite açmak.")
open(os.path.join(HERE, '..', 'Docs', 'ASSET_MALIYET_RAPORU.md'), 'w', encoding='utf-8').write('\n'.join(L) + '\n')
print('rows', len(rows), 'lines', len(L))
