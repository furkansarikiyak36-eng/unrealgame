"""PIE: a pit down to bedrock and a side tunnel near the player, dug with shovel strokes (soil only), for a look.
py ".../pie_voxel_scene.py" -> Saved/pie_voxel_scene.json (pit centre, messages, chunk stats)."""
import json, os
import unreal
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'pie_voxel_scene.json')
res = {'msgs': []}
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
def sub(cls):
    for o in unreal.ObjectIterator(cls):
        if o.get_outer() == world: return o
if world:
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    ts = sub(unreal.ExcavationTerrainChunkSubsystem)
    ex = sub(unreal.ExcavationVolumeSubsystem)
    p = pawn.get_actor_location()
    c = unreal.Vector(p.x + 900, p.y + 300, 0)
    res['pit'] = [c.x, c.y]
    up = unreal.Vector(0, 0, 1)
    rock = 0
    for z in range(0, -320, -40):          # 2 x 2 m pit, stroke by stroke
        for dx in (-50, 0, 50):
            for dy in (-50, 0, 50):
                r = ex.apply_sphere_edit(unreal.Vector(c.x + dx, c.y + dy, z - 20), 45.0, unreal.ExcavationEditType.CARVE)
                rec = r[1] if isinstance(r, tuple) else r
                if rec and not isinstance(rec, bool):
                    rock += rec.get_editor_property('blocked_cell_count')
    side = unreal.Vector(1, 0, 0)
    for x in range(100, 500, 40):           # tunnel into the pit wall at 1.2 m depth
        ex.apply_sphere_edit(unreal.Vector(c.x + x, c.y, -110), 60.0, unreal.ExcavationEditType.CARVE)
    res['blocked_rock_cells'] = rock
    res['flushed'] = ts.flush_pending_rebuilds()
    chunks = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ExcavationTerrainChunkActor)
    res['chunks'] = len(chunks)
    res['tris_total'] = sum(a.get_editor_property('last_triangle_count') for a in chunks)
    res['mats'] = sorted(set(a.get_editor_property('dynamic_mesh_component').get_material(0).get_name() for a in chunks))
json.dump(res, open(OUT, 'w'), indent=1, ensure_ascii=False)
