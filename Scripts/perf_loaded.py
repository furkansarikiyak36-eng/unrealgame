"""Read only: how many assets from the other content folders are loaded in memory now. Writes Saved/perf_loaded.json."""
import collections, json, os
import unreal
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'perf_loaded.json')
counts = collections.Counter()
for cls in (unreal.Texture2D, unreal.StaticMesh, unreal.SkeletalMesh):
    for obj in unreal.ObjectIterator(cls):
        p = obj.get_path_name()
        if p.startswith('/Game/'):
            counts[(cls.__name__, p.split('/')[2])] += 1
json.dump({'{}|{}'.format(*k): v for k, v in counts.most_common()}, open(OUT, 'w'), indent=1)
unreal.log('[PerfLoaded] done')
