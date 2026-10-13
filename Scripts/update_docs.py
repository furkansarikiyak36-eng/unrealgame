"""
Docs/KOD_HARITASI.md ve Docs/TEST_DURUMU.md dosyalarını üretir.

  python Scripts/update_docs.py

KOD_HARITASI: Source/MyProject2 altındaki başlık dosyalarındaki sınıfları ve genel fonksiyonları, üstlerindeki açıklama
yorumlarıyla toplar. Web Claude .cpp okuyamaz; bu sayfa ona kodun ne yaptığını az token'la verir. Açıklaması olmayan
fonksiyonlar sayfanın sonunda listelenir: yorum yazılacak yerler.

TEST_DURUMU: Saved/TestRuns içindeki en son mekanik test ve Wood birim testi kayıtlarının özeti.
Kayıtları Scripts/run_tests.sh oluşturur (editör kapalıyken çalıştırılır).
"""
import glob
import os
import re
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'Source', 'MyProject2')
DOCS = os.path.join(ROOT, 'Docs')
RUNS = os.path.join(ROOT, 'Saved', 'TestRuns')

SKIP_FILES = {'MyProject2.h'}
MAX_ARGS = 90
MAX_DOC = 170


# ----------------------------------------------------------------------------- code map

def clean_comment(lines):
    text = ' '.join(l.strip().lstrip('*').strip() for l in lines if l.strip().lstrip('*').strip() and not l.strip().startswith('@'))
    text = re.sub(r'\s+', ' ', text).strip()
    if len(text) > MAX_DOC:
        cut = text[:MAX_DOC]
        text = (cut[:cut.rfind(' ')] if ' ' in cut else cut) + ' …'
    return text


def parse_header(path):
    """Returns [(kind, name, doc, [(signature, doc)])] for the classes and namespaces of one header."""
    with open(path, encoding='utf-8') as f:
        lines = f.read().split('\n')

    blocks = []
    comment = []
    in_block = False
    depth = 0
    stack = []  # (name, body_depth, access, is_namespace)
    statement = ''
    seen_code = False

    for raw in lines:
        s = raw.strip()
        if in_block:
            comment.append(s.rstrip('*/').strip() if '*/' in s else s)
            if '*/' in s:
                in_block = False
            continue
        if s.startswith('/**') or (s.startswith('/*') and not s.startswith('/**/')):
            body = s[3:] if s.startswith('/**') else s[2:]
            comment = [] if not body.strip() else [body.replace('*/', '')]
            in_block = '*/' not in s
            if not in_block:
                comment = [body.replace('*/', '')]
            continue
        if s.startswith('//'):
            if not seen_code:
                continue  # file header
            comment.append(s[2:].strip())
            continue
        if s == '':
            if not statement:
                comment = []
            continue
        seen_code = True

        # macros that sit between a comment and its declaration
        if re.match(r'^(UCLASS|USTRUCT|UENUM|GENERATED_BODY|UPROPERTY|UFUNCTION|DECLARE_\w+|MYPROJECT2_API\s*$)', s) and not statement:
            if s.startswith('DECLARE_'):
                comment = []
            continue

        statement = (statement + ' ' + s).strip()
        opens = s.count('(') - s.count(')')
        balanced = statement.count('(') == statement.count(')')
        ends = statement.endswith(';') or statement.endswith('{') or statement.endswith('}') or re.search(r'\)\s*(const)?\s*(override)?\s*(= 0)?\s*;', statement)
        if not balanced or not (ends or statement.endswith(':')):
            continue
        decl = statement
        statement = ''
        top = stack[-1] if stack else None

        # access labels
        label = re.match(r'^(public|private|protected)\s*:$', decl)
        if label and top:
            stack[-1] = (top[0], top[1], label.group(1), top[3])
            comment = []
            continue

        # class / struct / namespace openings
        m = re.match(r'^(?:class|struct)\s+(?:\w+_API\s+)?(\w+)(?:\s*final)?(?:\s*:\s*[^{;]+)?\s*\{?$', decl)
        ns = re.match(r'^namespace\s+(\w+)\s*\{?$', decl)
        if (m and not decl.endswith(';')) or ns:
            name = m.group(1) if m else ns.group(1)
            is_ns = bool(ns)
            default = 'public' if (is_ns or decl.startswith('struct')) else 'private'
            doc = clean_comment(comment)
            blocks.append(['namespace' if is_ns else ('struct' if decl.startswith('struct') else 'class'), name, doc, []])
            stack.append((name, depth + 1, default, is_ns))
            depth += decl.count('{')
            comment = []
            continue

        # declarations directly in a class or namespace body (not inside inline function bodies)
        if top and depth == top[1] and top[2] == 'public' and '(' in decl:
            if re.match(r'^(UE_|return|if|for|while|static_assert|typedef|using|friend|FORCEINLINE_DEBUGGABLE)', decl):
                pass
            else:
                sig = decl
                sig = re.sub(r'\{.*$', '', sig).strip().rstrip(';').strip()
                sig = re.sub(r'\b\w+_API\s+', '', sig)
                sig = re.sub(r'\s+', ' ', sig)
                if len(sig) > MAX_ARGS + 40:
                    sig = sig[:MAX_ARGS + 40] + ' …'
                for b in reversed(blocks):
                    if b[1] == top[0]:
                        b[3].append((sig, clean_comment(comment)))
                        break
        elif top and depth == top[1] and top[2] == 'public' and decl.rstrip(';').endswith(')') is False:
            pass
        comment = []
        depth += decl.count('{') - decl.count('}')
        while stack and depth < stack[-1][1]:
            stack.pop()
        if decl == '};' or decl == '}':
            while stack and depth < stack[-1][1]:
                stack.pop()
    return blocks


def build_code_map():
    headers = sorted(glob.glob(os.path.join(SRC, '*.h')) + glob.glob(os.path.join(SRC, 'Wood', '*.h')))
    out = ['# Kod haritası', '',
           f'Otomatik üretildi ({time.strftime("%d %B %Y %H:%M")}) — `python Scripts/update_docs.py`. Elle düzenlenmez.', '',
           'Her sınıf için genel (public) fonksiyonlar ve üstlerindeki açıklama. Kaynak `Source/MyProject2` (`Wood/` alt klasörü web Claude paketi).',
           'Açıklaması olmayanlar sayfanın sonunda: bunlar için başlık dosyasına yorum yazılmalı.', '']
    missing = []
    total_funcs = 0
    for path in headers:
        name = os.path.basename(path)
        if name in SKIP_FILES:
            continue
        rel = os.path.relpath(path, SRC).replace('\\', '/')
        blocks = [b for b in parse_header(path) if b[3] or b[2]]
        if not blocks:
            continue
        out.append(f'## {rel}')
        out.append('')
        for kind, cname, doc, funcs in blocks:
            out.append(f'**{cname}** ({kind})' + (f' — {doc}' if doc else ''))
            out.append('')
            plain = []
            for sig, fdoc in funcs:
                if ' override' in sig and not fdoc:
                    continue  # motorun çağırdığı kancalar (BeginPlay, Tick...)
                total_funcs += 1
                short = re.match(r'.*?(\w+)\s*\(', sig)
                fname = short.group(1) if short else sig
                if fdoc:
                    out.append(f'- `{sig}` — {fdoc}')
                else:
                    plain.append(fname)
                    missing.append(f'{cname}::{fname}')
            if plain:
                out.append('- Ayrıca (açıklamasız): ' + ', '.join(f'`{n}`' for n in dict.fromkeys(plain)))
            out.append('')
    out.append('## Açıklaması eksik fonksiyonlar')
    out.append('')
    out.append(f'{len(missing)} / {total_funcs} fonksiyonun açıklaması yok. Yukarıda sınıfların altında "Ayrıca" satırlarında isimleriyle listeli; isimleri kendini açıklayan getter fonksiyonları için sorun değil.')
    out.append('')
    return '\n'.join(out), total_funcs, len(missing)


# ----------------------------------------------------------------------------- test status

def newest(pattern):
    files = glob.glob(os.path.join(RUNS, pattern))
    return max(files, key=os.path.getmtime) if files else None


def read_text(path):
    with open(path, encoding='utf-8', errors='replace') as f:
        return f.read()


def build_test_status():
    out = ['# Test durumu', '',
           f'Otomatik üretildi ({time.strftime("%d %B %Y %H:%M")}) — `python Scripts/update_docs.py`. Kayıtlar `Saved/TestRuns/`; `Scripts/run_tests.sh` oluşturur.', '']

    mech = newest('mekanik_*.log')
    out.append('## Mekanik test (grafiksiz, `-MechanicsDemoQuit`)')
    out.append('')
    if mech:
        text = read_text(mech)
        result = re.findall(r'SONUÇ: (\d+) geçti, (\d+) kaldı', text)
        stamp = time.strftime('%d %B %Y %H:%M', time.localtime(os.path.getmtime(mech)))
        if result:
            passed, failed = result[-1]
            out.append(f'Son çalıştırma: **{passed} geçti, {failed} kaldı** ({stamp}, `{os.path.basename(mech)}`)')
        else:
            out.append(f'Son çalıştırma ({stamp}) sonuç satırı içermiyor: test yarıda kalmış olabilir.')
        out.append('')
        failed_lines = re.findall(r'\[MechanicsDemo\] KALDI\s+(.*)', text)
        if failed_lines:
            out.append('Kalan kontroller:')
            out.append('')
            for line in failed_lines:
                out.append(f'- {line.strip()[:240]}')
            out.append('')
        passed_lines = re.findall(r'\[MechanicsDemo\] GEÇTİ\s+(.*)', text)
        out.append(f'Geçen kontrollerin özeti ({len(passed_lines)}):')
        out.append('')
        for line in passed_lines:
            out.append(f'- {re.sub(chr(10), " ", line.strip())[:170]}')
        out.append('')
        goals = len(re.findall(r'Görev tamam', text))
        out.append(f'Rehberli demo görevleri otomatik oynatmada tamamlanan: {goals}')
        out.append('')
        runs = sorted(glob.glob(os.path.join(RUNS, 'mekanik_*.log')), key=os.path.getmtime)[-6:]
        out.append('Son çalıştırmalar:')
        out.append('')
        for path in runs:
            r = re.findall(r'SONUÇ: (\d+) geçti, (\d+) kaldı', read_text(path))
            out.append(f'- {os.path.basename(path)}: ' + (f'{r[-1][0]} geçti, {r[-1][1]} kaldı' if r else 'sonuç yok'))
        out.append('')
    else:
        out.append('Henüz kayıtlı çalıştırma yok. `Scripts/run_tests.sh` çalıştırın.')
        out.append('')

    wood = newest('wood_*.log')
    out.append('## Wood birim testleri (`MyProject2.Wood`)')
    out.append('')
    if wood:
        text = read_text(wood)
        ok = re.findall(r'Test Completed\. Result=\{Success\} Name=\{(\w+)\} Path=\{([\w.]+)\}', text)
        bad = re.findall(r'Test Completed\. Result=\{Fail\} Name=\{(\w+)\} Path=\{([\w.]+)\}', text)
        stamp = time.strftime('%d %B %Y %H:%M', time.localtime(os.path.getmtime(wood)))
        out.append(f'Son çalıştırma: **{len(ok)} geçti, {len(bad)} kaldı** ({stamp})')
        out.append('')
        for name, path in ok:
            out.append(f'- ✅ {path}')
        for name, path in bad:
            out.append(f'- ❌ {path}')
        out.append('')
    else:
        out.append('Henüz kayıtlı çalıştırma yok.')
        out.append('')
    return '\n'.join(out)


def main():
    os.makedirs(DOCS, exist_ok=True)
    code_map, funcs, missing = build_code_map()
    with open(os.path.join(DOCS, 'KOD_HARITASI.md'), 'w', encoding='utf-8') as f:
        f.write(code_map)
    with open(os.path.join(DOCS, 'TEST_DURUMU.md'), 'w', encoding='utf-8') as f:
        f.write(build_test_status())
    size = os.path.getsize(os.path.join(DOCS, 'KOD_HARITASI.md')) / 1024
    print(f'KOD_HARITASI.md {size:.1f} KB, {funcs} fonksiyon, {missing} açıklamasız')
    print('TEST_DURUMU.md güncellendi')


if __name__ == '__main__':
    sys.exit(main())
