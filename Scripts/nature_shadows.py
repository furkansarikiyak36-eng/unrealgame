"""Turns shadow casting of the NatureLayer grass and cover HISMs on or off: py ".../nature_shadows.py" off|on"""
import json, os, sys
import unreal
on = (sys.argv[1] if len(sys.argv) > 1 else 'off') == 'on'
n = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if any(str(t) in ('NatureLayer_grass', 'NatureLayer_cover') for t in a.tags):
        for c in a.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent):
            c.set_cast_shadow(on); n += 1
json.dump({'cast_shadow': on, 'components': n}, open(os.path.join(unreal.Paths.project_saved_dir(), 'nature_shadows.json'), 'w'))
unreal.log('[NatureShadows] {} {}'.format(on, n))
