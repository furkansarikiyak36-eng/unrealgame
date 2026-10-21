"""Builds a self-contained HTML page with every /Game asset (sortable, filterable) from the inventory files.

Inputs: Saved/asset_inventory.json, Saved/mesh_dependencies.json, Saved/map_dependencies.json, Saved/asset_cost_per_mesh.csv
Output: the path given as the first argument (default Saved/asset_table.html).
Run outside the editor:  python Scripts/build_asset_table_page.py <out.html>
"""
import csv
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SAVED = os.path.join(HERE, '..', 'Saved')
INV = json.load(open(os.path.join(SAVED, 'asset_inventory.json')))
MAPS = json.load(open(os.path.join(SAVED, 'map_dependencies.json')))
MESH = {r['package']: r for r in csv.DictReader(open(os.path.join(SAVED, 'asset_cost_per_mesh.csv'), encoding='utf-8-sig'))}
MAIN, PT = set(MAPS['/Game/Maps/Main']), set(MAPS['/Game/Maps/PerfTest'])
BUNDLES = {b['pkg']: b for b in json.load(open(os.path.join(SAVED, 'asset_bundles.json')))}
CAPS = {}
try:
    for c in json.load(open(os.path.join(SAVED, 'cleanup_heavy.json')))['capped']:
        CAPS[c['texture']] = 2048
    for t, v in json.load(open(os.path.join(SAVED, 'restore_texture_caps.json'))):
        CAPS[t] = v
except FileNotFoundError:
    pass
BPP = {'DXT1': 0.5, 'BC4': 0.5, 'DXT3': 1, 'DXT5': 1, 'BC5': 1, 'BC7': 1, 'BC6H': 1, 'G8': 1, 'A8': 1, 'G16': 2, 'R16F': 2,
       'B8G8R8A8': 4, 'R8G8B8A8': 4, 'FloatRGBA': 8, 'A16B16G16R16': 8, 'A32B32G32R32F': 16}
NEW = {'SoulCave', 'KiteDemo', 'PN_MeadowFlowers', 'PN_Fern', 'PN_coniferBushes_I', 'PN_WildBerries', 'PN_tropicalGroundPlants'}

TYPE_TR = {'StaticMesh': 'Statik mesh', 'SkeletalMesh': 'Skeletal mesh', 'Texture2D': 'Doku', 'TextureCube': 'Küp doku',
           'Material': 'Materyal', 'MaterialInstanceConstant': 'Materyal örneği', 'MaterialFunction': 'Materyal fonksiyonu',
           'Blueprint': 'Blueprint', 'World': 'Harita', 'Skeleton': 'İskelet', 'PhysicsAsset': 'Fizik asset',
           'AnimSequence': 'Animasyon', 'AnimBlueprint': 'Animasyon BP', 'SoundWave': 'Ses', 'SoundCue': 'Ses',
           'ParticleSystem': 'Parçacık (Cascade)', 'NiagaraSystem': 'Niagara', 'FoliageType_InstancedStaticMesh': 'Foliage tipi',
           'PCGGraph': 'PCG grafiği', 'LandscapeGrassType': 'Landscape çim tipi', 'LandscapeLayerInfoObject': 'Landscape katmanı',
           'ObjectRedirector': 'Yönlendirici'}

rows = []
for r in INV:
    pkg = r['package']
    cls = r['class']
    use = ('M' if pkg in MAIN else '') + ('P' if pkg in PT else '')
    row = {'n': r['asset'], 't': TYPE_TR.get(cls, cls), 'p': r['pack'], 'd': round(r['disk_mb'], 2), 'u': use, 'new': r['pack'].split('/')[0] in NEW}
    note = ''
    if cls in ('StaticMesh', 'SkeletalMesh'):
        m = MESH.get(pkg, {})
        tris = int(float(m.get('tris_lod0') or r.get('tris') or 0))
        nan = (m.get('nanite') == 'evet')
        tex_mb = float(m.get('tex_mb') or 0)
        row.update({'c': m.get('category', ''), 'tri': tris, 'nan': nan, 'lod': m.get('lods') or '', 'ntri': int(float(m.get('nanite_tris') or 0)),
                    'tex': round(tex_mb, 1), 'tmax': int(float(m.get('tex_max') or 0)), 'mmb': float(m.get('mesh_mb') or 0),
                    'q': m.get('quality', '')})
        b = BUNDLES.get(pkg)
        if b:
            row.update({'fv': b['first_vram_mb'], 'bd': b['disk_mb'], 'mats': b['materials'], 'own': b['own_tex_mb'], 'sh': b['shared_tex_mb'],
                        'geo': b['geo_mb'], 'eo': b['editor_only_mb'], 'bones': b['bones'], 'ntri': b['ntris'],
                        'det': {'t': [[t['name'], t['dims'], t['eff'], t['fmt'], t['mb'], t['users']] for t in b['tex_list']],
                                'm': b['mat_list'], 'o': b['other']}})
            tex_mb = b['tex_mb']
        if cls == 'SkeletalMesh' and nan:
            row['gpu'] = 'Nanite'
            row['ram'] = '~7 MB (actor)'
        elif nan:
            row['gpu'] = 'Nanite'
            row['ram'] = '~0,2 KB'
        else:
            row['gpu'] = '{:.1f}k üçgen'.format(tris / 1000) if tris else '—'
            row['ram'] = '~0,2 KB'
        if not nan and tris > 50000:
            v, note = 'Ağır', 'Nanite kapalı, yakında {}k üçgen'.format(tris // 1000)
        elif tex_mb > 150:
            v, note = 'Ağır', 'Doku seti ~{:.0f} MB'.format(tex_mb)
        elif cls == 'SkeletalMesh' and nan:
            v, note = 'Orta', 'GPU ucuz; kopya başı ~7 MB RAM'
        elif (not nan and tris > 15000) or tex_mb > 50:
            v = 'Orta'
            note = 'Doku seti ~{:.0f} MB'.format(tex_mb) if tex_mb > 50 else '{}k üçgen'.format(tris // 1000)
        else:
            v = 'Hafif'
    elif r.get('dims') and 'x' in r['dims']:
        try:
            w, h = [int(x) for x in r['dims'].split('x')[:2]]
        except ValueError:
            w = h = 0
        f = r.get('format') or ''
        cap = CAPS.get(pkg) or 0
        ew, eh = w, h
        while cap and max(ew, eh) > cap:
            ew, eh = max(1, ew // 2), max(1, eh // 2)
        mb = ew * eh * BPP.get(f, 1) * 1.333 * (6 if cls == 'TextureCube' else 1) / 1048576.0
        w, h = ew, eh
        if cap and cap < max(int(x) for x in r['dims'].split('x')[:2]):
            note = '2K sınırı uygulandı ({} → {}x{}); '.format(r['dims'], ew, eh)
        row.update({'c': 'Doku', 'dim': r['dims'], 'fmt': f, 'tex': round(mb, 1), 'tmax': max(w, h), 'ns': r.get('never_stream') == 'True',
                    'vt': r.get('vt') == 'True', 'q': ('8K' if max(w, h) >= 8192 else '4K' if max(w, h) >= 4096 else '2K' if max(w, h) >= 2048
                                                        else '1K' if max(w, h) >= 1024 else '≤512')})
        unc = BPP.get(f, 1) >= 4 and max(w, h) > 64
        pre = note
        if mb >= 64 or (unc and max(w, h) >= 1024):
            v = 'Ağır'
            note = ('Sıkıştırılmamış ({}), '.format(f) if unc else '') + '{} ~{:.0f} MB'.format(r['dims'], mb)
        elif mb >= 16:
            v, note = 'Orta', '{} ~{:.0f} MB'.format(r['dims'], mb)
        else:
            v = 'Hafif'
        note = (pre + note) if pre and pre not in note else note
        if row['ns'] and max(w, h) > 64:
            note = (note + '; ' if note else '') + 'akışsız (hep tam yüklü)'
    else:
        row['c'] = row['t']
        v = 'Ağır' if r['disk_mb'] > 50 else 'Orta' if r['disk_mb'] > 5 else 'Hafif'
        if v != 'Hafif':
            note = 'Diskte {:.0f} MB'.format(r['disk_mb'])
    row['v'] = v
    row['note'] = note
    rows.append(row)

packs = {}
for x in rows:
    p = packs.setdefault(x['p'], {'p': x['p'], 'n': 0, 'd': 0.0, 'tex': 0.0, 'mesh': 0, 'heavy': 0, 'used': 0, 'new': x['new']})
    p['n'] += 1
    p['d'] += x['d']
    if x['t'] in ('Doku', 'Küp doku'):
        p['tex'] += x.get('tex', 0)
    if x['t'] in ('Statik mesh', 'Skeletal mesh'):
        p['mesh'] += 1
    p['heavy'] += x['v'] == 'Ağır'
    p['used'] += bool(x['u'])
pack_list = sorted(packs.values(), key=lambda p: -p['d'])
for p in pack_list:
    p['d'] = round(p['d'], 1)
    p['tex'] = round(p['tex'], 0)

data = json.dumps({'rows': rows, 'packs': pack_list}, ensure_ascii=False, separators=(',', ':'))
template = open(os.path.join(HERE, 'asset_table_template.html'), encoding='utf-8').read()
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(SAVED, 'asset_table.html')
open(out, 'w', encoding='utf-8').write(template.replace('/*DATA*/null', data))
print(out, len(rows), round(len(data) / 1048576, 2), 'MB data')
