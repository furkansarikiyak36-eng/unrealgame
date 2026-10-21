"""Actor classes, labels and instance totals in the open level: Saved/level_contents.json"""
import collections, json, os
import unreal
cls = collections.Counter(); inst = collections.Counter(); hidden = []
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    cls[a.get_class().get_name()] += 1
    for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
        inst[a.get_actor_label()] += c.get_instance_count()
    if a.is_hidden_ed():
        hidden.append(a.get_actor_label())
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
json.dump({'world': w.get_path_name(), 'classes': cls, 'instances': inst, 'hidden': hidden[:50]},
          open(os.path.join(unreal.Paths.project_saved_dir(), 'level_contents.json'), 'w'), indent=1)
