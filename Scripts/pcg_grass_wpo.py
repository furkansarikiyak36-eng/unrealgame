"""World Position Offset (wind) on the ISM components of GrassScatterVolume: py ".../pcg_grass_wpo.py" on|off
Measurement toggle; regenerating the PCG graph restores the spawner's own setting. Saved/pcg_grass_wpo.json"""
import json, os, sys
import unreal
on = (sys.argv[1] if len(sys.argv) > 1 else 'on') == 'on'
n = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label() == 'GrassScatterVolume':
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            if hasattr(c, 'set_evaluate_world_position_offset'):
                c.set_evaluate_world_position_offset(on)
            else:
                c.set_editor_property('evaluate_world_position_offset', on)
            n += 1
json.dump({'wpo': on, 'components': n}, open(os.path.join(unreal.Paths.project_saved_dir(), 'pcg_grass_wpo.json'), 'w'))
