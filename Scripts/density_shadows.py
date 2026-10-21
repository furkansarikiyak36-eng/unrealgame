"""Shadow casting of the DensityTest grass and cover HISMs: py ".../density_shadows.py" on|off"""
import json, os, sys
import unreal
on = (sys.argv[1] if len(sys.argv) > 1 else 'on') == 'on'
n = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if 'DensityTest' in [str(t) for t in a.tags] and a.get_actor_label() in ('Density_grass', 'Density_cover'):
        for c in a.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent):
            c.set_cast_shadow(on); n += 1
json.dump({'cast_shadow': on, 'components': n}, open(os.path.join(unreal.Paths.project_saved_dir(), 'density_shadows.json'), 'w'))
unreal.log('[DensityShadows] {} {}'.format(on, n))
