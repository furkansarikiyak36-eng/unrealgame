"""Splits every mesh into static scenery and dynamic (breakable / collectible) pieces, and into the cost-table kinds
(tree, bush, grass, ground plant, rock, wood, prop). Run outside the editor:  python Scripts/classify_mesh_roles.py
Inputs: Saved/asset_bundles.json (write_bundle_report.py), Saved/mesh_sizes.json (mesh_sizes.py, ApproxSize in cm).
Output: Saved/mesh_roles.json / .csv  {package: {role, kind, size_cm, reason}}
Roles: Dinamik-kırılır (trees that can be felled), Dinamik-toplanır (logs, wood, stones and props up to PICKUP_CM,
bushes and harvestable herbs), Statik (grass, other ground plants, big rocks, cliffs, architecture, tree clusters/cards),
Efekt (Paragon/SoulCave FX meshes), Karakter.
"""
import csv, json, os, re
SAVED = os.path.join(os.path.dirname(__file__), '..', 'Saved')
B = json.load(open(os.path.join(SAVED, 'asset_bundles.json'), encoding='utf-8'))
SIZE = json.load(open(os.path.join(SAVED, 'mesh_sizes.json')))
PICKUP_CM = 150
GRASS = re.compile(r'grass|kikuyu|wheat|sedge|reed|lawn', re.I)
HERB = re.compile(r'raspberry|elderberry|nettle|sage|burdock|mushroom|bolete|puffball|wheat|yarrow', re.I)
PROP = re.compile(r'pickaxe|shovel|scythe|pitchfork|scissors|rope|spool|stake|anvil|bucket|barrel|crate|basket|lantern|mushroom|bolete|puffball|bottle|jar|pot', re.I)
NOT_SINGLE_TREE = re.compile(r'merge|cluster|cards?|floating|huge', re.I)
out = {}
for b in B:
    p, cat, name = b['pkg'], b['cat'], b['name']
    if cat == 'Karakter' and b['class'] != 'Skeletal':
        cat = 'Yapı / eşya / diğer'   # e.g. Paragon "Dusk_" pieces: "sk_" in the name misled the category
    s = SIZE.get(p) or [0, 0, 0]
    big = max(s)
    fam = p.split('/')[5] if p.startswith('/Game/Fab/Megascans/') and len(p.split('/')) > 5 else name
    fx = '/FX/' in p or '/Effects' in p or 'EffectsMobile' in p
    if b['class'] == 'Skeletal':
        role, kind, why = 'Karakter', 'Karakter', 'karakter / skeletal'
    elif fx:
        role, kind, why = 'Efekt', 'Efekt', 'efekt mesh’i (parçacık sistemi kullanır)'
    elif cat == 'Ağaç':
        kind = 'Ağaç'
        if NOT_SINGLE_TREE.search(name) or big > 4000:
            role, why = 'Statik', 'ağaç kümesi/kartı veya dev arka plan ağacı'
        else:
            role, why = 'Dinamik-kırılır', 'tek ağaç: kesilir, kütük bırakır'
    elif cat == 'Kütük / odun':
        kind = 'Kütük / odun'
        if big > 1500:
            role, why = 'Statik', 'çok büyük ahşap parça ({:.0f} cm)'.format(big)
        else:
            role, why = 'Dinamik-toplanır', 'kütük/odun/dal: taşınır, kesilir'
    elif cat == 'Kaya / taş':
        kind = 'Kaya / taş'
        if big <= PICKUP_CM:
            role, why = 'Dinamik-toplanır', 'küçük taş (≤{} cm)'.format(PICKUP_CM)
        else:
            role, why = 'Statik', 'büyük kaya/kayalık ({:.0f} cm)'.format(big)
    elif cat == 'Çalı':
        kind, role, why = 'Çalı', 'Dinamik-toplanır', 'çalı: meyve/dal toplanır'
    elif cat.startswith('Çim'):
        if GRASS.search(fam) or GRASS.search(name):
            kind = 'Çim'
        else:
            kind = 'Zemin bitkisi'
        if HERB.search(fam) or HERB.search(name):
            role, why = 'Dinamik-toplanır', 'hasat edilebilir bitki'
        else:
            role, why = 'Statik', 'zemin örtüsü (dekor)'
    else:
        kind = 'Yapı / eşya'
        if re.search(r'mushroom|bolete|puffball', fam, re.I):
            kind = 'Zemin bitkisi'
            role, why = 'Dinamik-toplanır', 'mantar: toplanır'
            out[p] = {'name': name, 'pack': b['pack'], 'family': fam, 'kind': kind, 'role': role, 'size_cm': s, 'reason': why}
            continue
        if (PROP.search(fam) or PROP.search(name)) and 0 < big <= 300:
            role, why = 'Dinamik-toplanır', 'taşınır eşya / alet'
        else:
            role, why = 'Statik', 'yapı/büyük eşya ({:.0f} cm)'.format(big)
    out[p] = {'name': name, 'pack': b['pack'], 'family': fam, 'kind': kind, 'role': role, 'size_cm': s, 'reason': why}
json.dump(out, open(os.path.join(SAVED, 'mesh_roles.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=0)
with open(os.path.join(SAVED, 'mesh_roles.csv'), 'w', newline='', encoding='utf-8-sig') as f:
    w = csv.writer(f); w.writerow(['package', 'name', 'pack', 'family', 'kind', 'role', 'size_cm', 'reason'])
    for p, r in sorted(out.items()):
        w.writerow([p, r['name'], r['pack'], r['family'], r['kind'], r['role'], 'x'.join('{:.0f}'.format(v) for v in r['size_cm']), r['reason']])
import collections
c = collections.Counter((r['role'], r['kind']) for r in out.values())
for k, v in sorted(c.items()):
    print(v, *k)
