"""Felling test in a running PIE session (PerfTest).
py ".../pie_fell_test.py" fell [tallest|nearest]   stands 1.2 m from that standing tree and swings until it falls
py ".../pie_fell_test.py" logs                      lists raw logs (length from mesh x scale, position) and stumps
py ".../pie_fell_test.py" split <length>            stands by a raw log of that length and swings until it splits
Report: Saved/pie_fell_<mode>.json"""
import json, math, os, sys
import unreal
mode = sys.argv[1] if len(sys.argv) > 1 else 'logs'
arg = sys.argv[2] if len(sys.argv) > 2 else ''
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
out = {'mode': mode, 'pie': bool(world)}
def raw_length(h):
    m = h.get_component_by_class(unreal.StaticMeshComponent).get_editor_property('static_mesh')
    n = m.get_name() if m else ''
    if not n.startswith('SM_RawLog_'):
        return None
    return int(round(int(n.split('_')[2]) * h.get_actor_scale3d().x))
def stand_and_chop(target, max_swings, done):
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    sub = next(o for o in unreal.ObjectIterator(unreal.TreeFellingSubsystem) if o.get_outer() == world)
    t = target.get_actor_location(); p = pawn.get_actor_location()
    d = unreal.Vector(p.x - t.x, p.y - t.y, 0.0); d = d * (1.0 / max(d.length(), 1.0))
    reach = 120.0
    stand = unreal.Vector(t.x + d.x * reach, t.y + d.y * reach, t.z + 100.0)
    yaw = math.degrees(math.atan2(-d.y, -d.x))
    pawn.set_actor_location_and_rotation(stand, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw), False, True)
    ctrl = unreal.GameplayStatics.get_player_controller(world, 0)
    if ctrl:
        ctrl.set_control_rotation(unreal.Rotator(roll=0.0, pitch=-12.0, yaw=yaw + 35.0))
    swings = 0
    while swings < max_swings and not done():
        sub.chop_from(pawn); swings += 1
    return swings
if world:
    harv = list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HarvestableActor))
    trees = [h for h in harv if raw_length(h) is None and h.get_component_by_class(unreal.CapsuleComponent).get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION]
    logs = [h for h in harv if raw_length(h) is not None]
    if mode == 'fell' and trees:
        pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
        if arg == 'tallest':
            tgt = max(trees, key=lambda h: h.get_actor_bounds(False)[1].z)
        else:
            tgt = min(trees, key=lambda h: (h.get_actor_location() - pawn.get_actor_location()).length())
        out['target'] = [tgt.get_name(), tgt.get_component_by_class(unreal.StaticMeshComponent).get_editor_property('static_mesh').get_name(),
                         round(tgt.get_actor_bounds(False)[1].z * 2 / 100, 1)]
        cap = tgt.get_component_by_class(unreal.CapsuleComponent)
        out['swings'] = stand_and_chop(tgt, 12, lambda: cap.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION)
    if mode == 'split' and logs:
        want = int(arg)
        cand = [h for h in logs if raw_length(h) == want]
        if cand:
            tgt = cand[0]; before = len(logs)
            out['target'] = [tgt.get_name(), want]
            out['swings'] = stand_and_chop(tgt, 6, lambda: not unreal.SystemLibrary.is_valid(tgt))
            out['split'] = not unreal.SystemLibrary.is_valid(tgt)
        else:
            out['error'] = 'no log of length {}'.format(want)
    logs = [h for h in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HarvestableActor) if raw_length(h) is not None]
    out['logs'] = sorted([raw_length(h), round(h.get_actor_location().x), round(h.get_actor_location().y), round(h.get_actor_location().z)] for h in logs)
    out['stumps'] = [[round(a.get_actor_location().x), round(a.get_actor_location().y), round(a.get_actor_location().z), round(a.get_actor_scale3d().x, 2)]
                     for a in unreal.GameplayStatics.get_all_actors_with_tag(world, 'TreeStump')]
    out['standing_trees'] = len(trees)
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'pie_fell_{}.json'.format(mode)), 'w'), indent=1)
