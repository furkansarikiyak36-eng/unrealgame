"""Medians of one CSV profiler capture appended to Saved/perftest_results.json: python perf_parse.py label pose file"""
import csv, json, os, statistics, sys
label, pose, path = sys.argv[1:4]
rows = list(csv.reader(open(path, encoding='utf-8', errors='ignore')))
head = rows[0]
data = [r for r in rows[1:] if len(r) > len(head) // 2]
keys = ['FrameTime', 'GameThreadTime', 'RenderThreadTime', 'GPUTime', 'RHIThreadTime', 'RHI/DrawCalls',
        'RHI/PrimitivesDrawn', 'GPUSceneInstanceCount', 'GPUMem/LocalUsedMB', 'PhysicalUsedMB', 'DrawCall/ShadowDepths',
        'DrawCall/Basepass']
res = {'label': label, 'pose': pose, 'frames': len(data), 'csv': os.path.basename(path)}
for k in keys:
    if k in head:
        i = head.index(k)
        v = []
        for r in data:   # the file ends with a repeated header and metadata rows
            try:
                v.append(float(r[i]))
            except (ValueError, IndexError):
                pass
        if v:
            res[k] = round(statistics.median(v), 2)
out = os.path.join(os.path.dirname(__file__), '..', 'Saved', 'perftest_results.json')
allr = json.load(open(out)) if os.path.exists(out) else []
allr = [r for r in allr if not (r['label'] == label and r['pose'] == pose)] + [res]
json.dump(allr, open(out, 'w'), indent=1)
print(json.dumps(res))
