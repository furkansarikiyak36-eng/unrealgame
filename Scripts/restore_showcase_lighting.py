"""KitShowcase: rebuilds the default template lighting (sun, sky atmosphere, sky light, height fog, clouds) if missing,
in the "Lighting" folder like Template_Default. The 'Lighting' content group was renamed to 'LightingKit' to keep the
two apart. py ".../restore_showcase_lighting.py" -> Saved/restore_showcase_lighting.json"""
import json, os
import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/KitShowcase.'):
    raise RuntimeError('Open /Game/Maps/KitShowcase first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
have = {a.get_class().get_name() for a in actors.get_all_level_actors()}
made = []
def spawn(cls, loc=unreal.Vector(0, 0, 0), rot=unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0)):
    a = actors.spawn_actor_from_class(cls, loc, rot); a.set_folder_path('Lighting'); made.append(cls.__name__); return a
if 'DirectionalLight' not in have:
    sun = spawn(unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(roll=0.0, pitch=-40.0, yaw=-60.0))
    c = sun.get_component_by_class(unreal.DirectionalLightComponent)
    c.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
    c.set_editor_property('intensity', 10.0)
    c.set_editor_property('atmosphere_sun_light', True)
if 'SkyAtmosphere' not in have:
    spawn(unreal.SkyAtmosphere)
if 'SkyLight' not in have:
    sl = spawn(unreal.SkyLight, unreal.Vector(0, 0, 300))
    c = sl.get_component_by_class(unreal.SkyLightComponent)
    c.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
    c.set_editor_property('real_time_capture', True)
if 'ExponentialHeightFog' not in have:
    spawn(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 100))
if 'VolumetricCloud' not in have:
    spawn(unreal.VolumetricCloud)
for a in actors.get_all_level_actors():
    if a.get_actor_label().startswith('Label_Lighting'):
        a.get_component_by_class(unreal.TextRenderComponent).set_editor_property('text', 'LightingKit (1)')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/KitShowcase')
json.dump({'had': sorted(have), 'made': made}, open(os.path.join(unreal.Paths.project_saved_dir(), 'restore_showcase_lighting.json'), 'w'), indent=1)
