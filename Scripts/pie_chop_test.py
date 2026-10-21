"""Felling test in a running PIE session of PerfTest.
py ".../pie_chop_test.py" hit [n]   puts the player 1.2 m in front of the nearest fellable tree and swings n times (default 6)
py ".../pie_chop_test.py" status    counts trees, logs and sticks
py ".../pie_chop_test.py" look [m]  stands the player m metres (default 5) to the side of the nearest harvestable, facing it
Report: Saved/pie_chop_<mode>.json"""
import json, math, os, sys
import unreal
mode = sys.argv[1] if len(sys.argv) > 1 else 'status'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
out = {'mode': mode, 'pie': bool(world)}
if world:
    sub = next((o for o in unreal.ObjectIterator(unreal.TreeFellingSubsystem) if o.get_outer() == world), None)
    out['converted'] = sub.get_converted_count() if sub else None
    harv = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HarvestableActor)
    out['harvestables'] = len(harv)
    out['harvestable_list'] = sorted('{} {:.0f},{:.0f},{:.0f} pitch {:.0f}'.format(h.get_name()[:28], h.get_actor_location().x, h.get_actor_location().y,
                                    h.get_actor_location().z, h.get_actor_rotation().pitch) for h in harv)
    out['pickups'] = len(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PickupItem)) if hasattr(unreal, 'PickupItem') else None
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    out['pawn'] = pawn.get_class().get_name() if pawn else None
    if mode == 'look' and pawn and harv:
        dist = (float(sys.argv[2]) if len(sys.argv) > 2 else 5.0) * 100.0
        p = pawn.get_actor_location()
        tgt = min(harv, key=lambda h: (h.get_actor_location() - p).length())
        t = tgt.get_actor_location()
        fwd = tgt.get_actor_forward_vector()
        side = unreal.Vector(-fwd.y, fwd.x, 0.0)
        stand = unreal.Vector(t.x + side.x * dist, t.y + side.y * dist, p.z)
        yaw = math.degrees(math.atan2(t.y - stand.y, t.x - stand.x))
        pawn.set_actor_location_and_rotation(stand, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw), False, True)
        ctrl = unreal.GameplayStatics.get_player_controller(world, 0)
        if ctrl:
            ctrl.set_control_rotation(unreal.Rotator(roll=0.0, pitch=-15.0, yaw=yaw))
        out['target'] = tgt.get_name()
    if mode == 'hit' and pawn and harv:
        n = int(sys.argv[2]) if len(sys.argv) > 2 else 6
        p = pawn.get_actor_location()
        tree = min(harv, key=lambda h: (h.get_actor_location() - p).length())
        t = tree.get_actor_location()
        d = unreal.Vector(p.x - t.x, p.y - t.y, 0.0)
        d = d * (1.0 / max(d.length(), 1.0))
        stand = unreal.Vector(t.x + d.x * 120.0, t.y + d.y * 120.0, p.z)
        yaw = math.degrees(math.atan2(-d.y, -d.x))
        pawn.set_actor_location_and_rotation(stand, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw), False, True)
        ctrl = unreal.GameplayStatics.get_player_controller(world, 0)
        if ctrl:
            ctrl.set_control_rotation(unreal.Rotator(roll=0.0, pitch=-10.0, yaw=yaw))
        out['target'] = tree.get_name()
        out['hits'] = [sub.chop_from(pawn) for _ in range(n)]
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'pie_chop_{}.json'.format(mode)), 'w'), indent=1)
