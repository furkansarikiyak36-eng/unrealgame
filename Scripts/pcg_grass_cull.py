"""Cull distance of the ISM components on GrassScatterVolume (measurement only, regeneration restores the spawner's):
py ".../pcg_grass_cull.py" <end_m> [start_m]   0 = no culling. Saved/pcg_grass_cull.json"""
import json, os, sys
import unreal
end = float(sys.argv[1]) * 100.0
start = float(sys.argv[2]) * 100.0 if len(sys.argv) > 2 else end * 0.8
n = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label() == 'GrassScatterVolume':
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            c.set_cull_distances(int(start) if end else 0, int(end)); n += 1
json.dump({'end_cm': end, 'start_cm': start if end else 0, 'components': n}, open(os.path.join(unreal.Paths.project_saved_dir(), 'pcg_grass_cull.json'), 'w'))
