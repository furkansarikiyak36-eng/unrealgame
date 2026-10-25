"""Reads the asset registry tags stored in the Megaplant .uasset files (no editor needed, nothing is changed).

python Scripts/scan_megaplant_tags.py  ->  Saved/megaplant_tags.json
Per pack: the tree skeletal meshes (Nanite triangles of the whole assembled tree, bones), every part in Instances
(Nanite and fallback triangles, approximate size, Nanite compressed bytes) grouped by kind (Branch, Leaf, Twig,
Needle, Bark, Decoration...), every texture (dimensions) and the pack's size on disk.
"""
import json
import os
import re

ROOT = os.path.join(os.path.dirname(__file__), '..', 'Content', 'Megaplant_Library')
OUT = os.path.join(os.path.dirname(__file__), '..', 'Saved', 'megaplant_tags.json')
# A tag in the package summary: name, then an FString (int32 length, characters, terminating zero).
TAG = re.compile(rb'([A-Za-z][A-Za-z0-9]{2,40})\x00.{4}([ -~]{1,60})\x00', re.S)
KEEP = {'NaniteTriangles', 'NaniteVertices', 'Triangles', 'Vertices', 'ApproxSize', 'NaniteCompressedSize',
        'Bones', 'LODs', 'NaniteEnabled', 'Dimensions', 'Format', 'CompressionSettings', 'LODGroup'}


def tags(path):
    data = open(path, 'rb').read()
    found = {}
    for m in TAG.finditer(data):
        key = m.group(1).decode()
        if key in KEEP and key not in found:
            found[key] = m.group(2).decode().strip()
    cls = 'SkeletalMesh' if b'SkeletalMesh\x00' in data and 'Bones' in found else \
          'StaticMesh' if 'NaniteTriangles' in found or 'ApproxSize' in found else \
          'Texture' if 'Dimensions' in found else 'Other'
    return cls, found, len(data)


def kind(name):
    for k in ('Branch', 'Leaf', 'Twig', 'Needle', 'Bark', 'Decoration', 'Flower', 'Berry', 'Stem', 'Root', 'Cone'):
        if k.lower() in name.lower():
            return k
    return 'Other'


def main():
    report = {}
    for pack in sorted(os.listdir(ROOT)):
        base = os.path.join(ROOT, pack)
        entry = {'disk_mb': 0, 'trees': {}, 'parts': {}, 'skm_parts': 0, 'textures': {}}
        for dirpath, _, files in os.walk(base):
            for name in files:
                path = os.path.join(dirpath, name)
                entry['disk_mb'] += os.path.getsize(path) / 1048576
                if not name.endswith('.uasset'):
                    continue
                short = name[:-7]
                cls, found, _ = tags(path)
                in_instances = 'Instances' in dirpath
                if cls == 'SkeletalMesh' and in_instances:
                    entry['skm_parts'] += 1
                elif cls == 'SkeletalMesh':
                    entry['trees'][short] = found
                elif cls == 'StaticMesh':
                    found['kind'] = kind(short)
                    entry['parts'][short] = found
                elif cls == 'Texture':
                    entry['textures'][short] = found.get('Dimensions')
        entry['disk_mb'] = round(entry['disk_mb'], 1)
        report[pack] = entry
    with open(OUT, 'w') as f:
        json.dump(report, f, indent=1)
    print('wrote', os.path.abspath(OUT))


main()
