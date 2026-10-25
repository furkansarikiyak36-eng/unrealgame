"""Removes the scatter test layers from /Game/Maps/PerfTest so the kit work runs on a light scene:
the PCG grass (PCGVolume, PCGWorldActor, PCG partition actors), shrubs (AreaShrubs), Megaplant tree actors
(ShrublandTrees), the Megascans grass patch (GrassPatch) and any other PCG volume. Ground, lights, player start,
the third-person setup and the KitSamples row stay. Assets (PCG graphs, meshes) are not touched.
py ".../remove_perftest_scatter.py"   Report: Saved/remove_perftest_scatter.json
"""
import collections
import json
import os

import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
TAGS = {'AreaShrubs', 'ShrublandTrees', 'GrassPatch', 'PCGNature', 'DensityTest', 'Shrubland'}
removed = collections.Counter()
for a in actors.get_all_level_actors():
    tags = {str(t) for t in a.tags}
    cls = a.get_class().get_name()
    if (tags & TAGS) or cls in ('PCGVolume', 'PCGWorldActor', 'PCGPartitionActor') or a.get_actor_label() in ('GrassScatterVolume', 'AreaShrubs'):
        removed[a.get_actor_label() if cls in ('PCGVolume', 'PCGWorldActor') else (sorted(tags & TAGS) or [cls])[0]] += 1
        actors.destroy_actor(a)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/PerfTest')
left = collections.Counter(a.get_class().get_name() for a in actors.get_all_level_actors())
json.dump({'removed': removed, 'left': left}, open(os.path.join(unreal.Paths.project_saved_dir(), 'remove_perftest_scatter.json'), 'w'), indent=1)
