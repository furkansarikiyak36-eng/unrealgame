"""Editor: adds a temporary LogShowcase (not saved) to the open MechanicsTest, 80 m beside the fill. See probe_showcase.sh."""
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Maps/MechanicsTest')
s = eas.spawn_actor_from_class(unreal.LogShowcase, unreal.Vector(0, 8000, 0), unreal.Rotator(roll=0, pitch=0, yaw=0))
s.set_actor_label('TmpShowcase')
s.set_editor_property('build_on_play', False)
