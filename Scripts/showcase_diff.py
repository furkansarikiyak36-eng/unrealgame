"""Editor: what the user changed in the baked LogShowcase compared with the last bake (Saved/bake_showcase.json): moved,
turned, rescaled, added or removed pieces, per station. py ".../showcase_diff.py" -> Saved/showcase_diff.json"""
import json, math, os
import unreal

OUT = os.path.join(unreal.Paths.project_saved_dir(), 'showcase_diff.json')
SRC = os.path.join(unreal.Paths.project_saved_dir(), 'bake_showcase.json')
res = {'moved': [], 'missing': [], 'extra': []}
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
res['map'] = world.get_outer().get_path_name()
data = json.load(open(SRC, encoding='utf-8'))
actors = {a.get_actor_label(): a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()}


def station(x):
    return int(round(x / 1400.0)) + 1


for kind, items in (('ShowcaseLog', data['logs']), ('ShowcasePart', data['parts'])):
    for i, it in enumerate(items):
        label = '%s_%03d' % (kind, i)
        a = actors.get(label)
        x = it['xf']
        if not a:
            res['missing'].append({'label': label, 'station': station(x[0]), 'mesh': it['mesh'].split('/')[-1]})
            continue
        l = a.get_actor_location(); r = a.get_actor_rotation(); s = a.get_actor_scale3d()
        d = [l.x - x[0], l.y - x[1], l.z - x[2]]
        dr = [r.roll - x[3], r.pitch - x[4], r.yaw - x[5]]
        dr = [((v + 180) % 360) - 180 for v in dr]
        ds = [s.x - x[6], s.y - x[7], s.z - x[8]]
        if max(abs(v) for v in d) > 0.5 or max(abs(v) for v in dr) > 0.5 or max(abs(v) for v in ds) > 0.01:
            res['moved'].append({'label': label, 'station': station(x[0]), 'mesh': it['mesh'].split('/')[-1],
                                 'was': [round(v, 1) for v in x[:6]], 'move': [round(v, 1) for v in d],
                                 'turn': [round(v, 1) for v in dr], 'scale': [round(v, 3) for v in ds]})
known = set('ShowcaseLog_%03d' % i for i in range(len(data['logs']))) | set('ShowcasePart_%03d' % i for i in range(len(data['parts'])))
for label, a in actors.items():
    if isinstance(a, unreal.StaticMeshActor) and label not in known and label != 'Floor':
        l = a.get_actor_location(); r = a.get_actor_rotation()
        m = a.static_mesh_component.static_mesh
        res['extra'].append({'label': label, 'station': station(l.x), 'mesh': m.get_name() if m else None,
                             'at': [round(l.x, 1), round(l.y, 1), round(l.z, 1), round(r.roll, 1), round(r.pitch, 1), round(r.yaw, 1)],
                             'folder': str(a.get_folder_path())})
res['counts'] = {k: len(v) for k, v in res.items() if isinstance(v, list)}
json.dump(res, open(OUT, 'w'), indent=1)
