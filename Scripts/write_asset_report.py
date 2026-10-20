"""Builds Docs/ASSET_RAPORU.md from Saved/asset_inventory.json, asset_inventory_summary.json and map_dependencies.json.
Run outside the editor: python Scripts/write_asset_report.py"""
import collections
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
SAVED = os.path.join(HERE, '..', 'Saved')
R = json.load(open(os.path.join(SAVED, 'asset_inventory.json')))
S = json.load(open(os.path.join(SAVED, 'asset_inventory_summary.json')))
D = json.load(open(os.path.join(SAVED, 'map_dependencies.json')))
MAIN, PT = set(D['/Game/Maps/Main']), set(D['/Game/Maps/PerfTest'])
BPP = {'DXT1': 0.5, 'BC4': 0.5, 'DXT5': 1, 'BC5': 1, 'BC7': 1, 'BC6H': 1, 'G8': 1, 'G16': 2, 'B8G8R8A8': 4, 'FloatRGBA': 8}
for r in R:
    if r.get('dims') and 'x' in r['dims']:
        w, h = [int(x) for x in r['dims'].split('x')[:2]]
        r['w'], r['h'] = w, h
        r['gpu_mb'] = w * h * BPP.get(r.get('format') or '', 1) * 1.333 * (6 if r['class'] == 'TextureCube' else 1) / 1048576.0


def use(p):
    return ', '.join(x for x, s in (('Main', MAIN), ('PerfTest', PT)) if p in s) or '—'


packs = collections.OrderedDict()
for r in R:
    p = packs.setdefault(r['pack'], collections.Counter())
    p['n'] += 1
    p['disk'] += r['disk_mb']
    p['main'] += r['package'] in MAIN
    if r['class'] in ('Texture2D', 'TextureCube') and 'gpu_mb' in r:
        p['tex'] += 1
        p['tex_gpu'] += r['gpu_mb']
        p['tex4k'] += max(r['w'], r['h']) >= 4096
        p['tex_main_gpu'] += r['gpu_mb'] if r['package'] in MAIN else 0
    if r['class'] == 'StaticMesh':
        p['sm'] += 1
        if r.get('nanite'):
            p['nan'] += 1
        else:
            p['plain'] += r.get('tris') or 0
            p['heavy'] += (r.get('tris') or 0) > 50000
    if r['class'] == 'SkeletalMesh':
        p['sk'] += 1
        p['ntris'] += r.get('nanite_tris') or 0


def verdict(p):
    if p['packs'] > 1:      # a group of small Megascans packs: each one is light on its own
        return 'Hafif' if not p['heavy'] else 'Orta'
    if p['tex_gpu'] > 1000 or p['heavy'] >= 3 or p['disk'] > 1000:
        return 'Ağır'
    if p['tex_gpu'] > 150 or p['heavy'] or p['disk'] > 100 or p['ntris'] > 20e6:
        return 'Orta'
    return 'Hafif'


groups = collections.OrderedDict()
for k, p in packs.items():
    key = k
    if k.startswith('Fab/Megascans/') and p['disk'] < 50 and not p['heavy']:
        key = 'Fab/Megascans/' + k.split('/')[2] + ' (küçük paketler)'
    g = groups.setdefault(key, collections.Counter())
    g.update(p)
    g['packs'] += 1

NOTES = {
    'OldWest': "Main'in en büyük yükü: 140 adet 4K doku. 2K'ya sınırlanırsa ~4 kat küçülür.",
    'ProceduralNtr_vol2': "1,6 GB HDRI ve 682 MB FloatRGBA zemin dokusu kullanılmıyor; çam ağaçları ~260k üçgen, Nanite kapalı.",
    'ProceduralNaturePack': "207 MB HDRI kullanılmıyor; large_tree_w_roots_a 132k üçgen, Nanite kapalı.",
    'B3D': "8K RAW dokular (her biri 170-340 MB) kullanılmıyor; Main yalnız 8K normal haritasını kullanıyor (85 MB).",
    'PN_GrassLibrary': "Mesh'ler küçük; 'sorunlu' görünen 193 doku 18×2 gibi minik pivot painter verisi. Diskteki asıl yük 923 MB zemin dokuları.",
    'Megaplant_Library/Tree_Japanese_Cypress': "Cypress 01_A 83 M Nanite üçgen (kullanılmıyor). Displacement dokuları sıkıştırılmamış 4096×8192.",
    'Megaplant_Library/Tree_European_Aspen': "Displacement dokusu sıkıştırılmamış 4096×8192 (170 MB).",
    'Megaplant_Library/Tree_Norway_Maple': "Displacement dokusu sıkıştırılmamış 4096×8192 (170 MB).",
    'UnrealExport': "Kütükler: 58 mesh, toplam ~12k üçgen. Çok hafif.",
}

L = []
A = L.append
tot = S['totals']
A('# Asset raporu — hangisi maliyetli, hangisi değil (2026-10-19)\n')
A('Kaynak: asset kayıt etiketleri (`Scripts/inventory_assets.py`, asset yüklenmeden), harita bağımlılıkları '
  '(`Scripts/map_dependencies.py`); rapor `Scripts/write_asset_report.py` ile üretildi. Ham veri: `Saved/asset_inventory.csv`.\n')
A('## Önce bilinmesi gerekenler\n')
A('- **Bir asset ancak bir haritada kullanılırsa oyunda maliyet yaratır.** Kullanılmayan asset yalnız disk ve editör açılış süresi yer.')
A('- **Doku tahmini = tam çözünürlük + tüm mip\'ler.** Unreal dokuları gerektiği kadar akıtır; gerçek VRAM bundan az olur ve akış '
  'havuzuyla (~1 GB) sınırlıdır. Yine de "bu paket ne kadar ağır" sorusunun iyi bir ölçüsü.')
A('- **Mesh üçgeni:** Nanite mesh\'lerde üçgen sayısı çizimi az etkiler. Nanite olmayanlarda en detaylı LOD (LOD0) önemli; 50 binin üstü "ağır" sayıldı.')
A('- **Skeletal (Megaplant) ağaçlar:** GPU\'da Nanite sayesinde ucuz, ama her biri ayrı actor ve RAM yer (editörde ~500-700 ağaç sınırı).\n')
A('## Genel tablo\n')
A('| | Asset | Disk |')
A('|---|---|---|')
A('| Projedeki tüm asset\'ler | {} | {:.1f} GB |'.format(tot['assets'], tot['disk_mb'] / 1024))
A('| Main haritasının kullandıkları | {} paket | {:.1f} GB |'.format(len(MAIN), sum(r['disk_mb'] for r in R if r['package'] in MAIN) / 1024))
A('| PerfTest haritasının kullandıkları | {} paket | {:.1f} GB |'.format(len(PT), sum(r['disk_mb'] for r in R if r['package'] in PT) / 1024))
A('')
A('Doku: {} adet, tam çözünürlükte toplam ~{:.1f} GB. Statik mesh: {} ({} Nanite). Skeletal mesh: {}.\n'.format(
    tot['textures'], sum(r.get('gpu_mb', 0) for r in R) / 1024, tot['static_meshes'], tot['nanite_static'], tot['skeletal']))
A('## Paketler (diske göre)\n')
A('| Paket | Karar | Disk MB | Main\'de kullanılan / toplam asset | Doku (4K+) | Doku tam çöz. MB | Main\'deki doku MB | Statik mesh (Nanite) | Nanite\'sız LOD0 üçgen | >50k mesh | Skeletal | Not |')
A('|---|---|---|---|---|---|---|---|---|---|---|---|')
for k, p in sorted(groups.items(), key=lambda x: -x[1]['disk']):
    if p['disk'] < 5 and not p['heavy']:
        continue
    A('| {} | {} | {:.0f} | {} / {} | {} ({}) | {:.0f} | {:.0f} | {} ({}) | {:.0f}k | {} | {} | {} |'.format(
        k + (' ×{}'.format(p['packs']) if p['packs'] > 1 else ''), verdict(p), p['disk'], p['main'], p['n'],
        p['tex'], p['tex4k'], p['tex_gpu'], p['tex_main_gpu'], p['sm'], p['nan'], p['plain'] / 1000, p['heavy'],
        '{} ({:.0f} M Nanite)'.format(p['sk'], p['ntris'] / 1e6) if p['sk'] else '—', NOTES.get(k, '')))
A('\n5 MB\'tan küçük ve ağır mesh\'i olmayan paketler tabloya alınmadı (hepsi hafif).\n')
A('## En ağır Nanite\'sız mesh\'ler (LOD0 üçgen)\n')
A('| Üçgen | Mesh | Paket | LOD | Kullanan |')
A('|---|---|---|---|---|')
for r in [x for x in S['heavy_meshes'] if not x.get('nanite')][:20]:
    A('| {:.0f}k | {} | {} | {} | {} |'.format((r.get('tris') or 0) / 1000, r['asset'], r['pack'],
                                              int(r['lods']) if r.get('lods') else '?', use(r['package'])))
A('\nÇözüm: Nanite açmak ya da kullanmamak. Bitkilerde Nanite Foliage projede açık.\n')
A('## En ağır Nanite ağaçlar (skeletal)\n')
A('| Nanite üçgen | Ağaç | Disk MB | Kullanan |')
A('|---|---|---|---|')
for r in S['nanite_heavy'][:12]:
    A('| {:.1f} M | {} | {:.1f} | {} |'.format((r.get('nanite_tris') or 0) / 1e6, r['asset'], r['disk_mb'], use(r['package'])))
A('\nNanite\'ta üçgen sayısı çizimi sınırlı etkiler ama Nanite akış havuzunu (256 MB) ve diski doldurur.\n')
A('## En büyük dokular (tam çözünürlük tahmini)\n')
A('| MB | Doku | Boyut | Format | Paket | Kullanan |')
A('|---|---|---|---|---|---|')
for r in sorted([x for x in R if 'gpu_mb' in x], key=lambda x: -x['gpu_mb'])[:20]:
    A('| {:.0f} | {} | {} | {} | {} | {} |'.format(r['gpu_mb'], r['asset'], r['dims'], r.get('format') or '?', r['pack'], use(r['package'])))
A('\nFloatRGBA ve B8G8R8A8 sıkıştırılmamış (piksel başına 8 ve 4 bayt; DXT1 0,5, BC5/DXT5 1 bayt).\n')
A('## Sorunlu dokular (sıkıştırılmamış ya da akışsız, 64 pikselden büyük)\n')
A('| Doku | Boyut | Format | Akışsız | Paket | Kullanan |')
A('|---|---|---|---|---|---|')
for r in sorted([x for x in R if 'gpu_mb' in x and max(x['w'], x['h']) > 64 and
                 (BPP.get(x.get('format') or '', 1) >= 4 or x.get('never_stream') == 'True')], key=lambda x: -x['gpu_mb']):
    A('| {} | {} | {} | {} | {} | {} |'.format(r['asset'], r['dims'], r.get('format'), 'evet' if r.get('never_stream') == 'True' else '',
                                               r['pack'], use(r['package'])))
A('\n## Hafif ve güvenle kullanılabilir olanlar\n')
A('- **Megascans bitkileri ve yüzeyleri** (Low / tier 3 indirilmiş): paket başına ~8-12 MB, dokular küçük; bitkilerin çoğu 2-30k üçgen. '
  'İstisna: European Spindle VarC/VarD, Black Locust VarC, Bigleaf Hydrangea VarA/C (38-154k üçgen).')
A('- **UnrealExport kütükleri**: 58 mesh, toplam ~12k üçgen, 14 MB.')
A('- **Megascans 3D taş ve kayalar**: Nanite; çizim maliyeti düşük.')
A('- **PN_GrassLibrary çimleri**: 50-2.400 üçgen; maliyetleri ayarlardan (gölge, cull) geliyor, mesh\'ten değil.\n')
A('## Öneriler (kazanç sırasıyla)\n')
A('1. **Kullanılmayan dev dokuları sil** (disk ve editör): ProceduralNtr_vol2 HDRI (1,6 GB) ve forest_ground_debris_a_merge (682 MB), '
  'ProceduralNaturePack HDRI (207 MB), B3D 8K RAW setleri (~1,2 GB). Silmeden önce onay gerekir.')
A('2. **OldWest dokularını 2K\'ya sınırla** (Max Texture Size 2048 ya da LOD Bias 1): Main\'in doku yükü ~4 kat küçülür.')
A('3. **Nanite\'sız ağır mesh\'lere Nanite aç**: vol2 çam ağaçları (~260k), large_tree_w_roots_a (132k), Spindle VarC/D, Black Locust VarC, B3D odun yığınları (63-72k).')
A('4. **Megaplant displacement dokuları** (T_PVE_MeshDisplacement, 4096×8192, sıkıştırılmamış, 170 MB): ağaç üretim aracının verisi; '
  'oyunda VRAM\'de görülmedi (rhi dökümü), ama paketlenmiş oyunu şişirir. Paketlemeden önce kontrol edilmeli.')
A('5. **Kullanılmayan ağır ağaç varyantı**: Japanese Cypress 01_A (83 M Nanite üçgen) hiçbir haritada yok.')
open(os.path.join(HERE, '..', 'Docs', 'ASSET_RAPORU.md'), 'w', encoding='utf-8').write('\n'.join(L) + '\n')
print('ok', len(L))
