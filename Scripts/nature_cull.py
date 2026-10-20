"""Sets the end cull distance (cm) of one NatureLayer: py ".../nature_cull.py" <layer> <cm>"""
import json, os, sys
import unreal
layer, cm = sys.argv[1], float(sys.argv[2])
n = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if 'NatureLayer_' + layer in [str(t) for t in a.tags]:
        for c in a.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent):
            c.set_cull_distances(0, cm); n += 1
json.dump({'layer': layer, 'cull_cm': cm, 'components': n}, open(os.path.join(unreal.Paths.project_saved_dir(), 'nature_cull.json'), 'w'))
unreal.log('[NatureCull] {} {} {}'.format(layer, cm, n))
