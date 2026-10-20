"""Physical assembly test in a running PIE session of PerfTest with the project's character (AForestCharacter).
py ".../pie_assembly_test.py" state                 pawn, what it carries, shoulder load, last message, logs in the world
py ".../pie_assembly_test.py" pickup <length>       stands 1.6 m from a raw log of that length, looks at it, presses E
py ".../pie_assembly_test.py" press <e|lmb> [pitch] presses E or the left click looking ahead (pitch degrees, default -35)
py ".../pie_assembly_test.py" take                  looks at the nearest building piece (ACarvableLog) and presses E
py ".../pie_assembly_test.py" split <length>        swings the subsystem axe at a raw log of that length until it splits
Report: Saved/pie_assembly_<mode>.json"""
import json, math, os, sys
import unreal
mode = sys.argv[1] if len(sys.argv) > 1 else 'state'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
out = {'mode': mode, 'pie': bool(world)}
def raw_length(h):
    m = h.get_component_by_class(unreal.StaticMeshComponent).get_editor_property('static_mesh')
    n = m.get_name() if m else ''
    if not n.startswith('SM_RawLog_'):
        return None
    return int(round(int(n.split('_')[2]) * h.get_actor_scale3d().x))
def look_at(pawn, target_loc, dist, pitch=None):
    p = pawn.get_actor_location()
    d = unreal.Vector(p.x - target_loc.x, p.y - target_loc.y, 0.0); d = d * (1.0 / max(d.length(), 1.0))
    spot = unreal.Vector(target_loc.x + d.x * dist, target_loc.y + d.y * dist, p.z)
    yaw = math.degrees(math.atan2(-d.y, -d.x))
    pawn.set_actor_location_and_rotation(spot, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw), False, True)
    eye = spot.z + 60.0
    if pitch is None:
        pitch = -math.degrees(math.atan2(eye - target_loc.z, dist))
    ctrl = unreal.GameplayStatics.get_player_controller(world, 0)
    ctrl.set_control_rotation(unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw))
def state(pawn):
    harv = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HarvestableActor)
    raws = sorted(raw_length(h) for h in harv if raw_length(h) is not None)
    logs = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.CarvableLog)
    pieces = []
    for l in logs:
        o, e = l.get_actor_bounds(False)
        pieces.append([round(max(e.x, e.y, e.z) * 2), round(o.x), round(o.y), round(o.z)])
    s = {'pawn': pawn.get_class().get_name(), 'raw_logs_on_ground': raws, 'carvable_pieces': sorted(pieces)}
    if hasattr(pawn, 'test_carried_item'):
        s.update({'carried': pawn.test_carried_item(), 'count': pawn.test_carried_count(), 'load': round(pawn.test_carry_load(), 2),
                  'message': pawn.test_last_message()})
    return s
if world:
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    if mode == 'pickup':
        want = int(sys.argv[2])
        cand = [h for h in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HarvestableActor) if raw_length(h) == want]
        if cand:
            t = min(cand, key=lambda h: (h.get_actor_location() - pawn.get_actor_location()).length())
            look_at(pawn, t.get_actor_location(), 160.0)
            out['before'] = state(pawn)
            pawn.test_press_interact()
        else:
            out['error'] = 'no raw log of {}'.format(want)
    elif mode == 'goto':
        x, y = float(sys.argv[2]), float(sys.argv[3])
        pawn.set_actor_location(unreal.Vector(x, y, pawn.get_actor_location().z + 50.0), False, True)
    elif mode == 'take':
        logs = [l for l in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.CarvableLog)]
        if logs:
            t = min(logs, key=lambda l: (l.get_actor_location() - pawn.get_actor_location()).length())
            look_at(pawn, t.get_actor_location(), 170.0)
            out['target'] = [round(t.get_actor_location().x), round(t.get_actor_location().y), round(t.get_actor_location().z)]
            pawn.test_press_interact()
    elif mode == 'press':
        key = sys.argv[2] if len(sys.argv) > 2 else 'e'
        pitch = float(sys.argv[3]) if len(sys.argv) > 3 and sys.argv[3] != 'keep' else -35.0
        ctrl = unreal.GameplayStatics.get_player_controller(world, 0)
        r = ctrl.get_control_rotation()
        if sys.argv[3:4] != ['keep']:
            ctrl.set_control_rotation(unreal.Rotator(roll=0.0, pitch=pitch, yaw=r.yaw))
        (pawn.test_press_interact if key == 'e' else pawn.test_press_primary)()
    elif mode == 'split':
        want = int(sys.argv[2])
        sub = next(o for o in unreal.ObjectIterator(unreal.TreeFellingSubsystem) if o.get_outer() == world)
        cand = [h for h in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HarvestableActor) if raw_length(h) == want]
        if cand:
            t = cand[0]; look_at(pawn, t.get_actor_location(), 120.0)
            n = 0
            while n < 8 and unreal.SystemLibrary.is_valid(t):
                sub.chop_from(pawn); n += 1
            out['swings'] = n
    out['after'] = state(pawn)
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'pie_assembly_{}.json'.format(mode)), 'w'), indent=1, ensure_ascii=False)
