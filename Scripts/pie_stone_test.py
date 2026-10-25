"""Stone chain test in a running PIE session (PerfTest).
py ".../pie_stone_test.py" break [large|medium]   stands by the nearest stone of that size class and swings until it breaks
py ".../pie_stone_test.py" carve [medium|small]   stands by the nearest stone of that class and strikes the chisel until it changes
py ".../pie_stone_test.py" list                   stones: class (from mesh folder), size, position, physics
Report: Saved/pie_stone_<mode>.json"""
import json, math, os, sys
import unreal
mode = sys.argv[1] if len(sys.argv) > 1 else 'list'
want = sys.argv[2] if len(sys.argv) > 2 else 'large'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
out = {'mode': mode, 'pie': bool(world)}
def info(h):
    c = h.get_component_by_class(unreal.StaticMeshComponent)
    m = c.get_editor_property('static_mesh')
    path = m.get_path_name() if m else ''
    if '/Fab/Megascans/3D/' not in path:
        return None
    folder = path.split('/')[5]
    o, e = h.get_actor_bounds(False)
    bb = m.get_bounding_box(); sc = h.get_actor_scale3d()
    # same rule as AHarvestableActor: largest side of the mesh x largest scale
    size = round(max(bb.max.x - bb.min.x, bb.max.y - bb.min.y, bb.max.z - bb.min.z) * max(sc.x, sc.y, sc.z))
    kind = ('cobble' if 'Cobblestone' in folder else 'wall' if 'Castle_Wall' in folder else
            'large' if size >= 100 else 'medium' if size >= 40 else 'small')
    return {'name': h.get_name(), 'kind': kind, 'folder': folder[:30], 'size': size, 'loc': [round(o.x), round(o.y), round(o.z)],
            'physics': c.is_simulating_physics()}
def stand(target):
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    o, e = target.get_actor_bounds(False); p = pawn.get_actor_location()
    d = unreal.Vector(p.x - o.x, p.y - o.y, 0.0); d = d * (1.0 / max(d.length(), 1.0))
    r = max(e.x, e.y) + 70.0
    spot = unreal.Vector(o.x + d.x * r, o.y + d.y * r, o.z + 120.0)
    yaw = math.degrees(math.atan2(-d.y, -d.x))
    pawn.set_actor_location_and_rotation(spot, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw), False, True)
    ctrl = unreal.GameplayStatics.get_player_controller(world, 0)
    if ctrl:
        ctrl.set_control_rotation(unreal.Rotator(roll=0.0, pitch=-20.0, yaw=yaw + 30.0))
    return pawn
if world:
    sub = next(o for o in unreal.ObjectIterator(unreal.TreeFellingSubsystem) if o.get_outer() == world)
    stones = [(h, info(h)) for h in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HarvestableActor)]
    stones = [(h, i) for h, i in stones if i]
    if mode in ('break', 'carve'):
        pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
        skip = set(sys.argv[3].split(',')) if len(sys.argv) > 3 else set()
        cand = [(h, i) for h, i in stones if i['kind'] == want and not i['physics'] and i['name'] not in skip]
        if cand:
            h, i = min(cand, key=lambda hi: (hi[0].get_actor_location() - pawn.get_actor_location()).length())
            out['target'] = i
            pawn = stand(h)
            out['pawn_to_stone_cm'] = round((pawn.get_actor_location() - h.get_actor_location()).length())
            out['pawn_z'] = round(pawn.get_actor_location().z)
            mesh0 = h.get_component_by_class(unreal.StaticMeshComponent).get_editor_property('static_mesh')
            n = 0
            while n < 10 and unreal.SystemLibrary.is_valid(h) and h.get_component_by_class(unreal.StaticMeshComponent).get_editor_property('static_mesh') == mesh0:
                (sub.chop_from if mode == 'break' else sub.carve_from)(pawn); n += 1
            out['strikes'] = n
            out['gone'] = not unreal.SystemLibrary.is_valid(h)
            if unreal.SystemLibrary.is_valid(h):
                out['after'] = info(h)
        else:
            out['error'] = 'no {} stone'.format(want)
    stones = [i for i in (info(h) for h in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HarvestableActor)) if i]
    out['counts'] = {k: sum(1 for i in stones if i['kind'] == k) for k in ('large', 'medium', 'small', 'cobble', 'wall')}
    out['moving'] = sum(1 for i in stones if i['physics'])
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'pie_stone_{}.json'.format(mode)), 'w'), indent=1)
