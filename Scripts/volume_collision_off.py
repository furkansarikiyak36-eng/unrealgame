"""Turns collision off on every PCGVolume of PerfTest (their brush blocked WorldStatic traces: felled logs landed on the
volume's box, 8 m up). py ".../volume_collision_off.py" -> Saved/volume_collision_off.json"""
import json, os
import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first; open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))
done = []
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(a, unreal.PCGVolume):
        b = a.get_component_by_class(unreal.BrushComponent)
        b.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        b.set_collision_profile_name('NoCollision')
        done.append(a.get_actor_label())
unreal.EditorLoadingAndSavingUtils.save_map(_OPEN, '/Game/Maps/PerfTest')
json.dump(done, open(os.path.join(unreal.Paths.project_saved_dir(), 'volume_collision_off.json'), 'w'))
