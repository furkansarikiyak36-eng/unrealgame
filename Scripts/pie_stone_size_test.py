"""PIE: break a large rock with the subsystem pickaxe down to building stones, carve one, and report piece sizes.
py ".../pie_stone_size_test.py" -> Saved/pie_stone_size_test.json"""
import json, math, os
import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
out = {}
def sub(cls):
    for o in unreal.ObjectIterator(cls):
        if o.get_outer() == world: return o
def size(a):
    c = a.get_component_by_class(unreal.StaticMeshComponent)
    if not c or not c.static_mesh:
        return -1
    b = c.static_mesh.get_bounding_box()
    sc = a.get_actor_scale3d()
    return round(max((b.max.x - b.min.x) * sc.x, (b.max.y - b.min.y) * sc.y, (b.max.z - b.min.z) * sc.z))
def face(pawn, target, dist=120.0):
    t = target.get_actor_location(); p = pawn.get_actor_location()
    d = unreal.Vector(p.x - t.x, p.y - t.y, 0.0); d = d * (1.0 / max(d.length(), 1.0))
    o, e = target.get_actor_bounds(False)
    spot = unreal.Vector(t.x + d.x * (dist + max(e.x, e.y)), t.y + d.y * (dist + max(e.x, e.y)), p.z)
    yaw = math.degrees(math.atan2(-d.y, -d.x))
    pawn.set_actor_location_and_rotation(spot, unreal.Rotator(roll=0, pitch=0, yaw=yaw), False, True)
    unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(unreal.Rotator(roll=0, pitch=-20, yaw=yaw))
if world:
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    tf = sub(unreal.TreeFellingSubsystem)
    def is_rock(h):
        c = h.get_component_by_class(unreal.StaticMeshComponent)
        m = c.static_mesh if c else None
        return bool(m) and '/Megascans/3D/' in m.get_path_name()
    rocks = [h for h in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HarvestableActor) if is_rock(h)]
    out['rocks'] = sorted(size(r) for r in rocks)
    big = max(rocks, key=size)
    big_size = size(big)
    before = set(r.get_name() for r in rocks)
    face(pawn, big)
    hits = 0
    while big and hits < 40:
        if not unreal.SystemLibrary.is_valid(big):
            break
        tf.chop_from(pawn); hits += 1
    after = [h for h in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HarvestableActor) if is_rock(h) and h.get_name() not in before]
    out['big_size'] = big_size
    out['hits_large'] = hits
    out['medium_pieces'] = sorted(size(a) for a in after)
    out['medium_meshes'] = [a.get_component_by_class(unreal.StaticMeshComponent).static_mesh.get_name() for a in after]
    # break one medium piece
    if after:
        med = after[0]; before2 = set(h.get_name() for h in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HarvestableActor))
        face(pawn, med, 80.0); n = 0
        while n < 40:
            if not unreal.SystemLibrary.is_valid(med): break
            tf.chop_from(pawn); n += 1
        small = [h for h in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HarvestableActor) if h.get_name() not in before2 and is_rock(h)]
        out['building_stones'] = sorted(size(a) for a in small)
        out['med_broken'] = [size(med), n]
        out['building_meshes'] = [a.get_component_by_class(unreal.StaticMeshComponent).static_mesh.get_name() for a in small]
        if small:
            st = small[0]; face(pawn, st, 60.0)
            names = set(a.get_name() for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor))
            for k in range(4): tf.carve_from(pawn)
            new = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor) if a.get_name() not in names]
            out['carved'] = [[a.get_class().get_name(), size(a), a.get_component_by_class(unreal.StaticMeshComponent).static_mesh.get_name() if a.get_component_by_class(unreal.StaticMeshComponent) and a.get_component_by_class(unreal.StaticMeshComponent).static_mesh else None] for a in new][:5]
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'pie_stone_size_test.json'), 'w'), indent=1, default=str)
