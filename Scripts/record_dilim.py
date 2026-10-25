"""Turns Saved/house_run.txt (LogShowcase report) into Docs/DILIM_SONUC.md: every acceptance ID of Docs/DILIM_SOTF.md with
PASS / FAIL / BLOCKED, the stations that measured it, the date and the commit. The showcase measures that a piece was
placed the way the player's own code chooses (OK / XX per placement); extra acceptance checks (counts, volume, support) are
added as their own lines in the station and appear here as FAIL when they fail.
PASS = every station of the ID ran and had no XX.  FAIL = some XX.  BLOCKED = no station measures the ID yet, or it did not run."""
import re, subprocess, datetime, os, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RUN = os.path.join(ROOT, 'Saved', 'house_run.txt')
PLAN = os.path.join(ROOT, 'Docs', 'DILIM_SOTF.md')
OUT = os.path.join(ROOT, 'Docs', 'DILIM_SONUC.md')

# acceptance ID -> showcase stations that measure it (1-based: 1 cut chain, 2 wall, 3 cabin, 4 platform, 5 roof, 6 palisade,
# 7 diagonal, 8 steps, 9 furniture, 10 hauling, 11 module opening, 12 face sockets, 13 under sockets, 14 log house)
# ID -> (stations, regex on the placement line, what the criterion still needs beyond "placed the way the player's code chooses").
# A regex keeps unrelated placements of the same station out of the verdict. A text in the 3rd field means the station only
# measures placement: no XX gives KISMEN (partial), never PASS.
STATIONS = {
    'LOG-03': ([1], r'yarım|enine', 'parça sayısı ve hacim kontrolü'), 'LOG-04': ([1], r'çeyrek', 'parça sayısı ve hacim kontrolü'),
    'LOG-05': ([1], r'yarım|çeyrek', 'parça sayısı ve hacim kontrolü'), 'LOG-06': ([1], r'kalas|tahta|düzlen', 'parça sayısı ve hacim kontrolü'),
    'LOG-07': ([1], r'kalas|tahta', 'parça sayısı ve hacim kontrolü'), 'LOG-11': ([1], None, 'malzeme tüketimi / hacim kontrolü'),
    'LOG-13': ([3, 14], r'köşe|eşik', 'çentik/yuva geometrisi kontrolü'), 'LOG-14': ([1], None, 'taşınabilirlik kontrolü'),
    'ASM-01': ([1], None, None), 'ASM-02': ([4, 6, 16], r'direk|dikili|kazık', None), 'ASM-03': ([12, 13, 16], None, None),
    'ASM-04': ([3, 14], r'köşe', 'destek bağlantısı kontrolü'), 'ASM-05': ([2, 14, 16], r'sıra|eşik|üst sıra|alt sıra', None),
    'ASM-06': ([5, 7, 16], r'45|eğik|çapraz|yaslan', None), 'ASM-07': ([4, 15, 16], r'kiriş', None), 'ASM-08': ([4], r'payanda', None),
    'ASM-09': ([4, 9], r'döşeme|tahta', None), 'ASM-10': ([5, 14], r'çatı|mahya|eşik', None), 'ASM-11': ([8], None, 'oyuncu çıkabiliyor mu (yürüme) kontrolü'),
    'ASM-12': ([4], r'rampa', None), 'SUP-07': ([2, 11], r'kapı|pencere|açıklık|düş', 'üst sıraların desteği kontrolü'),
    'SUP-14': ([15], r'direği alınca|etkilenmedi|boşta|yerinde kaldı', None),
    'SUP-15': ([15, 16], r'yerinde kaldı', None),
    'DEF-01': ([6], None, 'çarpışma boşluğu kontrolü'), 'CAR-05': ([10], None, 'adet tutarlılığı kontrolü'),
    'CAR-06': ([10], None, 'adet tutarlılığı kontrolü'), 'CAR-07': ([10], None, 'adet tutarlılığı kontrolü'),
    'STO-01': ([10], r'raf', 'çift sayım kontrolü'), 'UTIL-10': ([9], None, None),
}

run = open(RUN, encoding='utf-8').read() if os.path.exists(RUN) else ''
stations = {}
cur = None
for line in run.splitlines():
    m = re.match(r'^(\d+)\.\s*(.*)', line)
    if m and not line.startswith('    '):
        cur = int(m.group(1)); stations[cur] = {'title': m.group(2)[:60], 'ok': 0, 'xx': [], 'lines': []}
        continue
    if cur and re.match(r'^\s+OK ', line):
        stations[cur]['ok'] += 1
        stations[cur]['lines'].append(('OK', line.strip()[3:].strip()))
    elif cur and re.match(r'^\s+XX ', line):
        stations[cur]['xx'].append(line.strip()[3:].strip()[:70])
        stations[cur]['lines'].append(('XX', line.strip()[3:].strip()))

def git(*a):
    try:
        return subprocess.run(['git', '-C', ROOT] + list(a), capture_output=True, text=True).stdout.strip()
    except Exception:
        return '?'
commit = git('rev-parse', '--short', 'HEAD') + (' (kirli çalışma kopyası)' if git('status', '--porcelain', '--untracked-files=no') else '')
branch = git('branch', '--show-current')

ids = []
for line in open(PLAN, encoding='utf-8'):
    m = re.match(r'^\|\s*([A-Z]+-\d+(?:[/–\.]+\d+)*)\s*\|\s*([^|]*)\|', line)
    if m:
        ids.append((m.group(1), m.group(2).strip()))

def verdict(i):
    spec = STATIONS.get(i)
    if not spec:
        return 'BLOCKED', "bu ID'yi ölçen istasyon yok"
    st, rx, missing = spec
    ran = [x for x in st if x in stations]
    if len(ran) < len(st):
        return 'BLOCKED', 'istasyon %s bu koşuda çalışmadı' % ','.join(str(x) for x in st if x not in stations)
    lines = [(k, t) for x in st for (k, t) in stations[x]['lines'] if not rx or re.search(rx, t, re.I)]
    bad = [t for k, t in lines if k == 'XX']
    ok = sum(1 for k, t in lines if k == 'OK')
    if bad:
        return 'FAIL', '; '.join(b[:60] for b in bad[:3]) + (' …' if len(bad) > 3 else '')
    if not lines:
        return 'BLOCKED', 'istasyon %s çalıştı ama bu ID için yerleştirme satırı bulunamadı' % ','.join(map(str, st))
    ev = 'istasyon %s: %d yerleştirme istenen gibi' % (','.join(map(str, st)), ok)
    if missing:
        return 'KISMEN', ev + '; eksik: ' + missing
    return 'PASS', ev

rows, counts = [], {'PASS': 0, 'KISMEN': 0, 'FAIL': 0, 'BLOCKED': 0}
for i, d in ids:
    v, e = verdict(i)
    counts[v] += 1
    rows.append('| %s | %s | **%s** | %s |' % (i, d[:60], v, e))

with open(OUT, 'w', encoding='utf-8') as f:
    f.write('# Dilim sonucu\n\n')
    f.write('Üreten: `Scripts/probe_showcase.sh` (Scripts/record_dilim.py). Plan: [DILIM_SOTF.md](DILIM_SOTF.md).\n\n')
    f.write('- Tarih: %s\n- Dal: %s, commit: %s\n- Çalışan istasyonlar: %s\n- Toplam: PASS %d, KISMEN %d, FAIL %d, BLOCKED %d\n\n' % (
        datetime.datetime.now().strftime('%Y-%m-%d %H:%M'), branch, commit, ', '.join(map(str, sorted(stations))) or 'yok',
        counts['PASS'], counts['KISMEN'], counts['FAIL'], counts['BLOCKED']))
    f.write('PASS: ölçen istasyon çalıştı, ilgili yerleştirmelerde XX yok ve ölçüt yalnız yerleştirmeyse. KISMEN: yerleştirme tamam, ölçütün geri kalanı (sayım, destek, yürüme...) henüz ölçülmüyor. FAIL: XX var. BLOCKED: ölçen istasyon yok ya da çalışmadı. '
            'Unreal içinde koşulmayan hiçbir satır PASS sayılmaz.\n\n')
    f.write('## Istasyonlar\n\n| # | İstasyon | OK | XX |\n|---|---|---|---|\n')
    for s in sorted(stations):
        f.write('| %d | %s | %d | %d |\n' % (s, stations[s]['title'], stations[s]['ok'], len(stations[s]['xx'])))
    f.write('\n## Kabul ID\'leri\n\n| ID | Davranış | Sonuç | Kanıt |\n|---|---|---|---|\n' + '\n'.join(rows) + '\n')
print('PASS %d  KISMEN %d  FAIL %d  BLOCKED %d  (istasyon: %s)' % (counts['PASS'], counts['KISMEN'], counts['FAIL'], counts['BLOCKED'], sorted(stations)))
