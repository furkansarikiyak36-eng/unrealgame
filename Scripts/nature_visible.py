"""Shows or hides whole NatureLayer actors (components leave the render scene): py ".../nature_visible.py" <layer> on|off"""
import json, os, sys
import unreal
layer, on = sys.argv[1], sys.argv[2] == 'on'
n = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if 'NatureLayer_' + layer in [str(t) for t in a.tags]:
        for c in a.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent):
            c.set_visibility(on); n += 1
json.dump({'layer': layer, 'visible': on, 'components': n}, open(os.path.join(unreal.Paths.project_saved_dir(), 'nature_visible.json'), 'w'))
unreal.log('[NatureVisible] {} {} {}'.format(layer, on, n))
