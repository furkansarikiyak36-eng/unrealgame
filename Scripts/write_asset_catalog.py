"""Asset catalogue for the Content Drawer: every mesh with a short Turkish description (what it is, its size), its mesh
form (triangles, Nanite, LODs, size) and its textures (count, kind, largest size, VRAM), plus a summary of the other
assets of each top folder. Run outside the editor:  python Scripts/write_asset_catalog.py
Inputs (refresh in the editor first when content changed): Saved/asset_inventory.json (inventory_assets.py),
Saved/asset_bundles.json (bundle_dependencies.py + write_bundle_report.py), Saved/mesh_roles.json
(classify_mesh_roles.py), Saved/mesh_sizes.json (mesh_sizes.py).
Output: Docs/AssetKatalogu/README.md (index) and one file per top folder.
Descriptions are built from the asset names with a word list (NAMES / WORDS below); unknown words are kept as they are.
"""
import collections
import json
import os
import re

ROOT = os.path.join(os.path.dirname(__file__), '..')
SAVED = os.path.join(ROOT, 'Saved')
OUT = os.path.join(ROOT, 'Docs', 'AssetKatalogu')

INV = json.load(open(os.path.join(SAVED, 'asset_inventory.json'), encoding='utf-8'))
BUNDLES = {b['pkg']: b for b in json.load(open(os.path.join(SAVED, 'asset_bundles.json'), encoding='utf-8'))}
ROLES = json.load(open(os.path.join(SAVED, 'mesh_roles.json'), encoding='utf-8'))
SIZES = json.load(open(os.path.join(SAVED, 'mesh_sizes.json')))

# What each top folder of the Content Drawer is.
FOLDERS = collections.OrderedDict([
    ('Maps', ('Haritalar', 'Projenin haritaları: Main (oyunun ana haritası), PerfTest (doğa demosu ve mekanik testleri, editörün açılış haritası), KitShowcase (tüm kit mesh\'lerinin sergisi).')),
    ('UnrealExport', ('Ham kütükler (kendi)', 'Blender\'dan aktarılan ham kütükler: 75 / 150 / 300 / 600 cm boylar ve işlenmiş halleri (yarım, kalas, oluk, yontulmuş, çentikli). Ağaç kesilince çıkan ve montajda kullanılan parçalar bunlar. M_RawLog materyali kabuk, kesik, yontulmuş ve soyulmuş yüzeyleri ile yıllık halkalı uç kesiti çizer. Not: düz SM_RawLog_75 bu envanterden sonra eklendi (150 cm kütüğün yarı boyu).')),
    ('LogWall', ('Kütük duvar (kendi)', 'Kütük duvar denemesinin parçaları.')),
    ('Building', ('Yapı (kendi)', 'Megascans eski kütüklerinden duvara uydurulmuş düz kütükler ve kesik ahşap materyali.')),
    ('PerfTest', ('Test içeriği (kendi)', 'PerfTest haritasının PCG çim grafları (PCG_FieldGrass, PCG_GrassScatter), zemin materyali ve üçüncü şahıs test karakteri (BP_TP_Character, manken, animasyonlar).')),
    ('Audio', ('Sesler (kendi)', 'Kesme / kırma / oyma sesleri (geçici, kod ile üretildi): SW_AxeChop (balta), SW_TreeCrack (gövde çatırdaması), SW_TreeFall (ağacın yere düşüşü), SW_PickaxeHit (kazma), SW_RockBreak (kaya kırılması), SW_StoneCarve (keski).')),
    ('Characters', ('Karakter', 'Oyunun kullandığı UE4 mankeni (Mannequin_OWF): iskelet, fizik, animasyon taslağı, 7 animasyon.')),
    ('Gameplay', ('Oynanış materyalleri', 'Oyun içi yardımcı materyaller (vurgu, hayalet önizleme vb.).')),
    ('Greybox', ('Greybox', 'Eski greybox içeriğinden kalan tek taş mesh\'i (diğerleri silindi).')),
    ('Fab', ('Megascans (Fab)', 'Quixel Megascans içerikleri, Low kademe (1K doku): 3D (kaya, taş, kütük, dal, mantar, alet), Plants (çim, çiçek, eğrelti, çalı, ağaç) ve Surfaces (döşenebilir zemin / duvar / kumaş yüzeyleri). Depoda yok, Fab\'dan indirilir.')),
    ('KiteDemo', ('KiteDemo (Epic)', 'Epic\'in "A Boy and His Kite" demosundan doğa kiti: İskoç çamı ve tepe ağaçları, tarama kayalar ve kayalık, eğrelti, çiçek, funda, mürt çalısı, zemin dokuları. Fotogerçekçi, dokular 2K\'ya sınırlandı. Depoda yok.')),
    ('ParagonProps', ('Paragon (Epic)', 'Epic\'in Paragon oyunundan çevre parçaları: Agora (taş tapınak, duvar, kemer, kuyu, kaya), Monolith (Dawn / Dusk / Ruins: fantastik mimari, harabe, kayalar), Turrets (taretler), FX (efekt mesh\'leri). Ağır dokulu; 2K\'ya sınırlandı. Depoda yok.')),
    ('SoulCave', ('Soul: Cave (Epic)', 'Mağara / yeraltı kiti: mağara kayaları ve sütunları, gecekondu yapıları, köprü, tırabzan, merdiven, mağara ağaçları, su ve şelale efektleri. Hafif, stilize-gerçekçi. Depoda yok.')),
    ('SampleMap', ('Paragon örnek harita', 'Paragon kitinin gün ışığı örnek haritası. Depoda yok.')),
    ('Lighting', ('Örnek ışık', 'Paragon örnek haritasının gökyüzü ve HDR ışık asset\'leri. Depoda yok.')),
])

CLASS_TR = {
    'StaticMesh': 'statik mesh', 'SkeletalMesh': 'iskeletli mesh', 'Texture2D': 'doku', 'TextureCube': 'küp doku (gökyüzü)',
    'Material': 'materyal', 'MaterialInstanceConstant': 'materyal örneği', 'MaterialFunction': 'materyal fonksiyonu',
    'MaterialParameterCollection': 'materyal parametre koleksiyonu', 'World': 'harita', 'MapBuildDataRegistry': 'harita ışık verisi',
    'Blueprint': 'blueprint', 'AnimBlueprint': 'animasyon blueprint', 'AnimSequence': 'animasyon', 'BlendSpace1D': 'animasyon karışımı',
    'AimOffsetBlendSpace1D': 'nişan karışımı', 'Skeleton': 'iskelet', 'PhysicsAsset': 'fizik asset', 'ParticleSystem': 'parçacık efekti',
    'SoundWave': 'ses', 'SoundCue': 'ses kurgusu', 'PCGGraph': 'PCG grafı', 'FoliageType_InstancedStaticMesh': 'foliage tipi',
    'PhysicalMaterial': 'fiziksel materyal', 'VectorFieldStatic': 'vektör alanı', 'CurveFloat': 'eğri', 'CurveLinearColor': 'renk eğrisi',
    'SubsurfaceProfile': 'yüzey altı profili',
}
KIND_TR = {'Ağaç': 'Ağaç', 'Çalı': 'Çalı', 'Çim': 'Çim', 'Zemin bitkisi': 'Zemin bitkisi', 'Kaya / taş': 'Kaya / taş',
           'Kütük / odun': 'Kütük / odun', 'Yapı / eşya': 'Yapı / eşya', 'Efekt': 'Efekt mesh\'i', 'Karakter': 'Karakter'}

# Whole names first (Megascans families and odd kit names), then single words.
NAMES = {
    'ScotsPine': 'İskoç çamı', 'ScotsPineTall': 'uzun İskoç çamı', 'HillTree': 'tepe ağacı (yapraklı)', 'HillTree Tall': 'uzun tepe ağacı',
    'Dragon Tree': 'ejder ağacı', 'Bigleaf Hydrangea': 'ortanca', 'Dead Shrubs': 'kuru çalı', 'Elderberry': 'mürver', 'Raspberry': 'ahududu',
    'Alexandra Palm': 'Alexandra palmiyesi (genç)', 'Beech Fern': 'kayın eğreltisi', 'Bog Marshcress': 'bataklık teresi',
    'Common Hemp Nettle': 'kenevir ısırganı', 'Curbside Weeds': 'yol kenarı yabani otu', 'Dry Grass': 'kuru ot', 'English Ivy': 'sarmaşık',
    'Field Poppy': 'gelincik', 'Greater Burdock': 'dulavratotu', 'Ground Cover': 'yer örtücü bitki', 'Kikuyu Grass': 'kikuyu çimi',
    'Lady Fern': 'dişi eğrelti', 'Lily of the Valley': 'inci çiçeği', 'Matted Pratia': 'yayılıcı pratia', 'Pink Cordyline': 'pembe kordilin',
    'Rembrandt Water Lily': 'nilüfer', 'Ribbon Grass': 'kurdele otu', 'Sage': 'adaçayı', 'Small Balsam': 'küçük kına çiçeği',
    'Thatching Grass': 'saz otu (dam otu)', 'Violet Wood Sorrel': 'mor ekşi yonca', 'Wheat Grass': 'buğday otu', 'Wild Grass': 'yabani çim',
    'Wood Sorrel': 'ekşi yonca', 'Yellow Archangel': 'sarı ballıbaba', 'Bolete Mushrooms': 'çörek mantarı', 'Puffball Mushrooms': 'puf mantarı',
    'Mossy Stones Pack': 'yosunlu taşlar', 'Small Stones Pack': 'küçük taşlar', 'Mossy Forest Boulder': 'yosunlu orman kayası',
    'Beach Boulder': 'sahil kayası', 'Beach Rock': 'sahil taşı', 'Nordic Beach Rock': 'kuzey sahil taşı', 'Nordic Beach Rocks': 'kuzey sahil kayaları',
    'Sandstone Boulder': 'kumtaşı kaya', 'Rock': 'kaya', 'Castle Wall Stone': 'kale duvarı taşı (düz kesme taş)',
    'Roadside Construction Cobblestone Brown Pack': 'kahverengi parke taşı (cobblestone)',
    'Roadside Construction Cobblestone Gray Pack': 'gri parke taşı (cobblestone)',
    'Military Trenches Scatter Rock S 10': 'dağınık küçük taş', 'Military Trenches Equipment Stick Wood S 01': 'ince sopa',
    'Military Trenches Equipment Stick Wood S 03': 'ince sopa', 'Military Trenches Equipment Stick Wood S 04': 'ince sopa',
    'Military Trenches Equipment Stake Wood': 'ahşap kazık', 'Broken Stump': 'kırık ağaç kütüğü (dip)', 'Old Tree Stump': 'eski ağaç kütüğü (dip)',
    'Oak Tree Log': 'meşe kütüğü', 'Old Wooden Beam': 'eski ahşap kiriş', 'Old Wooden Log': 'eski ahşap kütük', 'Tree Debris Pack': 'ağaç döküntüsü (dal, kabuk)',
    'Wood Debris Pack': 'odun döküntüsü', 'Wood Debris': 'odun döküntüsü', 'Wooden Beam': 'ahşap kiriş', 'Wooden Table': 'ahşap masa',
    'Ancient Temple Jungle Roots': 'antik tapınak orman kökleri', 'Blacksmith Anvil': 'demirci örsü', 'Modular Fence Post': 'çit direği',
    'Old Pickaxe': 'eski kazma', 'Old Pitchfork': 'eski yaba', 'Old Scythe': 'eski tırpan', 'Old Stove': 'eski soba', 'Old Well Crank': 'kuyu çıkrığı',
    'Rusty Sheep Scissors': 'paslı koyun makası', 'Rusty Shovel': 'paslı kürek', 'Small Rope Spool': 'küçük ip makarası',
}
WORDS = {
    'tree': 'ağaç', 'trees': 'ağaçlar', 'pine': 'çam', 'oak': 'meşe', 'aspen': 'kavak', 'bush': 'çalı', 'shrub': 'çalı', 'leaf': 'yaprak',
    'leaves': 'yapraklar', 'grass': 'çim', 'fern': 'eğrelti', 'flower': 'çiçek', 'flowers': 'çiçekler', 'plant': 'bitki', 'plants': 'bitkiler',
    'moss': 'yosun', 'mossy': 'yosunlu', 'heather': 'funda', 'bogmyrtle': 'bataklık mürtü', 'myrtle': 'mürt', 'yarrow': 'civanperçemi',
    'buttercup': 'düğünçiçeği', 'scabious': 'uyuz otu', 'fieldgrass': 'çayır otu', 'deadleaves': 'kuru yapraklar', 'dead': 'kuru',
    'rock': 'kaya', 'rocks': 'kayalar', 'boulder': 'iri kaya', 'stone': 'taş', 'stones': 'taşlar', 'cliff': 'uçurum / kayalık', 'scree': 'moloz yığını',
    'pebbles': 'çakıl', 'pebble': 'çakıl', 'gravel': 'çakıl', 'volcanic': 'volkanik', 'large': 'büyük', 'medium': 'orta', 'small': 'küçük',
    'mountain': 'dağ', 'rockface': 'kaya yüzü', 'groundreveal': 'yerden çıkan', 'reveal': 'açığa çıkan', 'river': 'dere', 'plains': 'ova',
    'log': 'kütük', 'logs': 'kütükler', 'stump': 'kütük dibi', 'branch': 'dal', 'twig': 'ince dal', 'root': 'kök', 'roots': 'kökler', 'wood': 'ahşap',
    'wooden': 'ahşap', 'plank': 'kalas', 'beam': 'kiriş', 'debris': 'döküntü', 'firewood': 'odun', 'bark': 'kabuk',
    'wall': 'duvar', 'walls': 'duvarlar', 'floor': 'zemin', 'ground': 'zemin', 'pillar': 'sütun', 'pillars': 'sütunlar', 'column': 'sütun',
    'stairs': 'merdiven', 'stair': 'merdiven', 'step': 'basamak', 'steps': 'basamaklar', 'arch': 'kemer', 'arches': 'kemerler', 'gate': 'kapı',
    'door': 'kapı', 'portal': 'portal', 'tower': 'kule', 'turret': 'taret', 'roof': 'çatı', 'window': 'pencere', 'bridge': 'köprü', 'ramp': 'rampa',
    'platform': 'platform', 'base': 'taban', 'core': 'çekirdek', 'ring': 'halka', 'piece': 'parça', 'pieces': 'parçalar', 'trim': 'kenar şeridi',
    'corner': 'köşe', 'straight': 'düz', 'end': 'uç', 'endcap': 'bitiş parçası', 'join': 'birleşim', 'broken': 'kırık', 'ruins': 'harabe',
    'ruin': 'harabe', 'remains': 'kalıntı', 'statue': 'heykel', 'pedestal': 'kaide', 'banner': 'sancak', 'lamp': 'lamba', 'sconce': 'duvar lambası',
    'brazier': 'mangal', 'deco': 'süs', 'decoration': 'süs', 'details': 'ayrıntı', 'metal': 'metal', 'concrete': 'beton', 'marble': 'mermer',
    'jungle': 'orman', 'well': 'kuyu', 'cylinder': 'silindir', 'block': 'blok', 'blockpiece': 'blok parçası', 'tile': 'karo', 'tiles': 'karolar',
    'spawner': 'doğma noktası', 'inhibitor': 'inhibitör', 'integrated': 'birleşik', 'whole': 'bütün', 'lower': 'alt', 'upper': 'üst', 'top': 'üst',
    'bottom': 'alt', 'mid': 'orta', 'middle': 'orta', 'center': 'orta', 'side': 'yan', 'flat': 'düz', 'round': 'yuvarlak', 'tall': 'uzun',
    'short': 'kısa', 'long': 'uzun', 'half': 'yarım', 'quarter': 'çeyrek', 'gutter': 'oluk', 'trough': 'yalak', 'hewn': 'yontulmuş',
    'peeled': 'soyulmuş', 'kalas': 'kalas', 'notch': 'çentik', 'flatnotch': 'düz çentik', 'cave': 'mağara', 'slums': 'gecekondu', 'slum': 'gecekondu',
    'building': 'bina', 'distance': 'uzak plan', 'wires': 'teller', 'wire': 'tel', 'chain': 'zincir', 'handrail': 'tırabzan', 'handrails': 'tırabzanlar',
    'road': 'yol', 'path': 'patika', 'water': 'su', 'waterfall': 'şelale', 'flow': 'akış', 'ripple': 'dalga', 'ripples': 'dalgalar', 'splash': 'sıçrama',
    'fog': 'sis', 'godray': 'ışık huzmesi', 'cards': 'kartlar (düz resim)', 'merge': 'birleştirilmiş', 'merged': 'birleştirilmiş', 'cluster': 'küme',
    'unified': 'birleşik', 'bricks': 'tuğlalar', 'brick': 'tuğla', 'plane': 'düzlem', 'hump': 'tümsek', 'warble': 'dalgalı', 'trash': 'çöp',
    'barrel': 'fıçı', 'crate': 'sandık', 'pipe': 'boru', 'rope': 'ip', 'fence': 'çit', 'post': 'direk', 'pole': 'direk', 'spire': 'kule ucu',
    'flowertree': 'çiçek ağacı', 'floating': 'yüzen', 'huge': 'dev', 'shockwave': 'şok dalgası', 'spline': 'eğri', 'splines': 'eğriler',
    'explosion': 'patlama', 'sphere': 'küre', 'disc': 'disk', 'cone': 'koni', 'beam_fx': 'ışın', 'lightcone': 'ışık konisi', 'lightrays': 'ışık ışınları',
    'shattered': 'parçalanmış', 'vortex': 'girdap', 'flame': 'alev', 'smoke': 'duman', 'plasma': 'plazma', 'hexgon': 'altıgen', 'plates': 'plakalar',
    'segment': 'bölüm', 'cycroom': 'stüdyo fonu', 'assetplatform': 'sergi platformu', 'arrows': 'oklar', 'skybox': 'gökyüzü küresi',
    'skyfloor': 'gökyüzü zemini', 'raw': 'ham', 'rawlog': 'ham kütük', 'stick': 'sopa', 'equipment': 'ekipman', 'military': 'askeri', 'trenches': 'siper',
    'scatter': 'dağınık', 'construction': 'inşaat', 'roadside': 'yol kenarı', 'temple': 'tapınak', 'ancient': 'antik', 'anvil': 'örs',
    'crosslap': 'çapraz geçme', 'saddle': 'eyer', 'cradle': 'beşik', 'mate': 'eş', 'fork': 'çatal', 'halflap': 'yarım geçme', 'notch_v': 'V çentik',
    'logsstraight': 'düz kütükler', 'straightened': 'düzleştirilmiş', 'fit': 'uyarlanmış', 'origends': 'orijinal uçlu', 'gb': 'greybox',
    'mannequin': 'manken', 'character': 'karakter', 'glow': 'parıltı', 'hit': 'vuruş', 'muzzle': 'namlu ağzı', 'bullet': 'mermi', 'dome': 'kubbe',
    'recall': 'geri çağırma', 'blast': 'patlama', 'chunks': 'parçalar', 'crunch': 'ezilme', 'impact': 'darbe', 'warp': 'bükülme',
    'electric': 'elektrik', 'complex': 'karmaşık', 'body': 'gövde', 'spirals': 'sarmallar', 'hammer': 'çekiç', 'heart': 'kalp', 'curve': 'eğri',
    'death': 'ölüm', 'flare': 'parlama', 'flares': 'parlamalar', 'tri': 'üçlü', 'angelsjon': 'Angelsjön (tarama kaya)', 'agelsjon': 'Agelsjön (tarama kaya)',
    'hadrianswall': 'Hadrian Duvarı taraması', 'burnhope': 'Burnhope (tarama kaya)', 'nordic': 'kuzey', 'rocknordic': 'kuzey kayası',
    'cliffrock': 'kayalık', 'leafpile': 'yaprak yığını', 'formation': 'oluşum', 'strip': 'şerit', 'entrance': 'giriş', 'big': 'büyük',
    'crumble': 'ufalanmış', 'nest': 'yuva', 'dino': 'dinozor', 'surround': 'çevre', 'riverrocks': 'dere kayaları', 'rocksbig': 'büyük kayalar',
    'hole': 'delik', 'support': 'destek', 'frame': 'çerçeve', 'cap': 'başlık', 'edge': 'kenar', 'small_': 'küçük', 'piece_': 'parça',
    'wing': 'kanat', 'crystal': 'kristal', 'orb': 'küre', 'globe': 'küre', 'bowl': 'kase', 'seat': 'oturak', 'bench': 'bank', 'table': 'masa',
    'chair': 'sandalye', 'cart': 'araba', 'wheel': 'tekerlek', 'sign': 'tabela', 'shelf': 'raf', 'box': 'kutu', 'bucket': 'kova',
    'vase': 'vazo', 'pot': 'çömlek', 'jar': 'kavanoz', 'candle': 'mum', 'torch': 'meşale', 'flag': 'bayrak', 'noflag': 'bayraksız',
    'flagpole': 'bayrak direği', 'roots_': 'kökler', 'vine': 'sarmaşık', 'vines': 'sarmaşıklar', 'mushroom': 'mantar', 'mushrooms': 'mantarlar', 'pile': 'yığın', 'aclove': 'Aclove', 'buff': 'güç', 'gold': 'altın', 'green': 'yeşil',
}
KEEP = {'Agora', 'Monolith', 'Dawn', 'Dusk', 'DawnSide', 'DuskSide', 'Terminus', 'TerminusCore', 'Soul', 'LV', 'OP', 'SM', 'S'}


def split_words(name):
    name = re.sub(r'^(SM|SK|S|HM)_', '', name)
    name = re.sub(r'_(LOD0_TIER3_\d+|tier_3|Var[A-Z]+\d*|\d+[a-z]?)$', '', name)
    parts = re.split(r'[_\s]+', name)
    words = []
    for part in parts:
        words += re.findall(r'[A-Z]+(?![a-z])|[A-Z]?[a-z]+|\d+', part) or [part]
    return [w for w in words if w]


def translate(name):
    if name in NAMES:
        return NAMES[name]
    words = split_words(name)
    joined = ''.join(w.lower() for w in words)
    if joined in WORDS:
        return WORDS[joined]
    out, i = [], 0
    while i < len(words):
        two = (words[i] + (words[i + 1] if i + 1 < len(words) else '')).lower()
        if i + 1 < len(words) and two in WORDS:
            out.append(WORDS[two]); i += 2; continue
        w = words[i]
        out.append(WORDS.get(w.lower(), w if (w in KEEP or not w.isalpha() or len(w) <= 2) else w))
        i += 1
    deduped = [w for k, w in enumerate(out) if k == 0 or w.lower() != out[k - 1].lower()]
    text = ' '.join(deduped)
    return text[:1].upper() + text[1:] if text else name


def family_label(pkg):
    """Megascans: the asset family (folder without its id), else the mesh name."""
    parts = pkg.split('/')
    if pkg.startswith('/Game/Fab/Megascans/') and len(parts) > 5:
        return re.sub(r'_[A-Za-z0-9]{5,10}$', '', parts[5]).replace('_', ' ')
    return parts[-1]


TEX_KIND = [(r'(_N|_Normal|Normal|_NRM|_NM|_N_Tex|_N_Bump|_normals|_n)$', 'normal'),
            (r'(_D|_B|_BC|_C|_Color|_Colored|_Diffuse|_Albedo|_albedo|_Tex|_D_R|_D_CC|_D_CC_R|_D2|_D2_R|_COLOR|_D_alt_R|_Atlas)$', 'renk'),
            (r'(_ORM|_ORT|_M|_Mask|_Masks|_R|_S|_AO|_H|_Roughness|_Specular|_Specular_Tex|_MZ|_E|_Height|_DispMask|_Mipfix|_A|_O)$', 'maske')]


def tex_kind(name):
    if 'Billboard' in name:
        return 'billboard'
    for pattern, kind in TEX_KIND:
        if re.search(pattern, name):
            return kind
    return 'yardımcı'


def fmt(v, d=0):
    s = ('{:,.%df}' % d).format(v)
    return s.replace(',', 'X').replace('.', ',').replace('X', '.')


def mesh_cell(rows):
    tris = [(b['ntris'] if b['nanite'] and b['ntris'] else b['tris']) for _, b in rows]
    nan = sum(1 for _, b in rows if b['nanite'])
    lods = {int(b['lods'] or 1) for _, b in rows}
    t = fmt(min(tris)) if min(tris) == max(tris) else '{}–{}'.format(fmt(min(tris)), fmt(max(tris)))
    bits = [t + ' üçgen']
    if nan:
        bits.append('Nanite' if nan == len(rows) else 'Nanite {}/{}'.format(nan, len(rows)))
    elif max(lods) > 1:
        bits.append('{} LOD'.format(max(lods)))
    if rows[0][1].get('class') == 'Skeletal':
        bits.append('iskeletli')
    return ', '.join(bits)


def size_text(pkgs):
    sizes = [SIZES.get(p) for p in pkgs if SIZES.get(p)]
    if not sizes:
        return ''
    if len(sizes) == 1:
        x, y, z = sizes[0]
        return '{}×{}×{} cm'.format(*(fmt(v) for v in (x, y, z)))
    longest = [max(s) for s in sizes]
    return 'en uzun kenar {}–{} cm'.format(fmt(min(longest)), fmt(max(longest)))


def tex_cell(rows):
    tex = {}
    for _, b in rows:
        for t in b['tex_list']:
            tex[t['pkg']] = t
    if not tex:
        return 'doku yok (düz renk / parametre)'
    kinds = collections.Counter(tex_kind(t['name']) for t in tex.values())
    biggest = max(max(int(v) for v in t['eff'].split('x')) for t in tex.values())
    mb = sum(t['mb'] for t in tex.values())
    kind_txt = ', '.join('{} {}'.format(n, k) for k, n in kinds.most_common())
    return '{} doku ({}), en büyük {} px, ~{} MB'.format(len(tex), kind_txt, biggest, fmt(mb, 1))


def describe(label, rows, pkgs):
    r = ROLES.get(pkgs[0], {})
    kind = KIND_TR.get(r.get('kind'), r.get('kind', ''))
    what = translate(label)
    size = size_text(pkgs)
    role = r.get('role', '')
    role_txt = {'Dinamik-kırılır': 'kesilir', 'Dinamik-toplanır': 'toplanır / taşınır', 'Statik': 'sabit', 'Efekt': 'efekt', 'Karakter': ''}.get(role, '')
    head = '{}: {}'.format(kind, what) if kind and kind.lower() not in what.lower() else what
    extra = [x for x in (size, role_txt) if x]
    return head + (' — ' + ', '.join(extra) if extra else '')


def section_for_meshes(top):
    pkgs = [p for p in BUNDLES if p.split('/')[2] == top]
    groups = collections.OrderedDict()
    for p in sorted(pkgs):
        parts = p.split('/')
        if top == 'Fab':
            sub = '/'.join(parts[3:5])
        elif top in ('ParagonProps', 'KiteDemo'):
            sub = '/'.join(parts[3:5]) if len(parts) > 5 else ('/'.join(parts[3:-1]) or '(kök)')
        elif top == 'SoulCave':
            sub = '/'.join(parts[3:-1]) or '(kök)'
        else:
            sub = '/'.join(parts[3:-1]) or '(kök)'
        fam = family_label(p)
        groups.setdefault(sub, collections.OrderedDict()).setdefault(fam, []).append(p)
    lines = []
    for sub, fams in groups.items():
        n = sum(len(v) for v in fams.values())
        lines.append('\n### {} ({} mesh)\n'.format(sub, n))
        lines.append('| Asset | Ne | Mesh | Doku |')
        lines.append('|---|---|---|---|')
        for fam, ps in fams.items():
            rows = [(p, BUNDLES[p]) for p in ps]
            name = fam if len(ps) == 1 else '{} ({} varyant)'.format(fam, len(ps))
            size = size_text(ps)
            lines.append('| `{}` | {} | {} | {} |'.format(name, describe(fam, rows, ps).replace('|', '/'),
                                                         mesh_cell(rows), tex_cell(rows)))
    return lines


def other_assets(top):
    rows = [r for r in INV if r['package'].split('/')[2] == top and r['class'] not in ('StaticMesh', 'SkeletalMesh')]
    c = collections.Counter(r['class'] for r in rows)
    if not c:
        return []
    out = ['\n### Diğer asset\'ler\n', ', '.join('{} {}'.format(n, CLASS_TR.get(k, k)) for k, n in c.most_common()) + '.']
    named = [r for r in rows if r['class'] in ('World', 'PCGGraph', 'Blueprint', 'AnimBlueprint', 'SoundWave', 'SoundCue', 'Material')
             and top not in ('ParagonProps', 'SoulCave', 'KiteDemo', 'Fab')]
    if named:
        out.append('')
        for r in sorted(named, key=lambda r: (r['class'], r['package'])):
            out.append('- `{}` — {}'.format(r['package'].replace('/Game/', ''), CLASS_TR.get(r['class'], r['class'])))
    tex = [r for r in rows if r['class'] == 'Texture2D']
    if tex and top in ('ParagonProps', 'SoulCave', 'KiteDemo', 'Fab'):
        out.append('\nDokular tek tek listelenmedi ({} adet); her mesh\'in kullandığı dokular yukarıdaki tabloda.'.format(len(tex)))
    return out


def main():
    os.makedirs(OUT, exist_ok=True)
    tops = collections.Counter(r['package'].split('/')[2] for r in INV)
    index = ['# Asset Kataloğu', '',
             'Content Drawer\'daki her klasör ve her mesh için kısa açıklama: **Ne** (ne olduğu, boyu, oyundaki rolü), '
             '**Mesh** (üçgen sayısı, Nanite / LOD) ve **Doku** (sayı, tür, en büyük boyut, video belleği). '
             'Megascans\'ta aynı bitki / taşın varyantları tek satırda. Doku türleri: *renk* (base color), *normal*, '
             '*maske* (pürüzlülük, AO, yükseklik, ORM vb.), *billboard* (uzak görünüm resmi).',
             '',
             'Kaynak: 20 Ekim 2026 envanteri (`Saved/asset_inventory.json`, `asset_bundles.json`, `mesh_roles.json`). '
             'Yenilemek için editörde envanter betikleri, sonra `python Scripts/write_asset_catalog.py`. '
             'Açıklamalar asset adlarından bir sözlükle çevrildi; sözlükte olmayan özel adlar (Agora, Dawn, Dusk...) olduğu gibi kaldı.',
             '', '| Klasör | Ne | Mesh | Asset |', '|---|---|---|---|']
    for top, (title, text) in FOLDERS.items():
        if top not in tops and top != 'Audio':
            continue
        n_mesh = sum(1 for p in BUNDLES if p.split('/')[2] == top)
        fname = '{}.md'.format(top)
        index.append('| [{}]({}) | {} | {} | {} |'.format(top, fname, title, n_mesh, tops.get(top, 6 if top == 'Audio' else 0)))
        lines = ['# {} — {}'.format(top, title), '', text, '', '[← Asset kataloğu](README.md)']
        lines += section_for_meshes(top)
        lines += other_assets(top)
        open(os.path.join(OUT, fname), 'w', encoding='utf-8').write('\n'.join(lines) + '\n')
    rest = sorted(set(tops) - set(FOLDERS))
    if rest:
        index += ['', 'Diğer küçük klasörler: ' + ', '.join(rest) + '.']
    index += ['', 'Not: Fab, KiteDemo, ParagonProps, SoulCave, SampleMap ve Lighting depoda yok (bkz. `Docs/RAPOR_2026-10-20.md`); '
              'katalog yine de onları anlatır, çünkü haritalar onları kullanıyor.']
    open(os.path.join(OUT, 'README.md'), 'w', encoding='utf-8').write('\n'.join(index) + '\n')
    print('ok', len(BUNDLES), 'mesh')


if __name__ == '__main__':
    main()
