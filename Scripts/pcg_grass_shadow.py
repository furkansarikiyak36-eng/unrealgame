"""Cast shadow of the ISM components on GrassScatterVolume (PCG output): py ".../pcg_grass_shadow.py" on|off
Measurement toggle only; regenerating the PCG graph restores the spawner's own setting."""
import json, os, sys
import unreal
on = (sys.argv[1] if len(sys.argv) > 1 else 'on') == 'on'
n = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label() == 'GrassScatterVolume':
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            c.set_cast_shadow(on); n += 1
json.dump({'cast_shadow': on, 'components': n}, open(os.path.join(unreal.Paths.project_saved_dir(), 'pcg_grass_shadow.json'), 'w'))
