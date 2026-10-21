"""Removes every test layer from PerfTest (PCG volumes, NatureLayer_*, PerfLayer_* except the ground, DensityTest) and
saves the map. Report: Saved/perf_clean.json."""
import collections, json, os
import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
removed = collections.Counter()
for a in actors.get_all_level_actors():
    tags = [str(t) for t in a.tags]
    hit = [t for t in tags if t in ('PCGNature', 'DensityTest') or t.startswith('NatureLayer_') or (t.startswith('PerfLayer_') and t != 'PerfLayer_base')]
    if hit or isinstance(a, (unreal.PCGVolume, unreal.PCGPartitionActor)):
        removed[hit[0] if hit else a.get_class().get_name()] += 1
        actors.destroy_actor(a)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorLoadingAndSavingUtils.save_map(w, '/Game/Maps/PerfTest')
json.dump({'removed': dict(removed), 'left': len(actors.get_all_level_actors())}, open(os.path.join(unreal.Paths.project_saved_dir(), 'perf_clean.json'), 'w'))
unreal.log('[PerfClean] done')
