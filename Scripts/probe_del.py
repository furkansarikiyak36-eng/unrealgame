"""Editor: removes the temporary LogShowcase again."""
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_actor_label() == 'TmpShowcase':
        eas.destroy_actor(a)
open(unreal.Paths.project_saved_dir() + '/tmp_show_gone.txt', 'w').write('ok')
