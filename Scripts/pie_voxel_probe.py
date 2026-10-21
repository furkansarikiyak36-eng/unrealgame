"""PIE: what the voxel excavation layer does now and what an edit costs (data + mesh + collision rebuild, synchronous).
py ".../pie_voxel_probe.py" -> Saved/pie_voxel_probe.json"""
import json, os, time
import unreal
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'pie_voxel_probe.json')
res = {}
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
def sub(cls):
    for o in unreal.ObjectIterator(cls):
        if o.get_outer() == world: return o
if world:
    ex = sub(unreal.ExcavationVolumeSubsystem)
    ts = sub(unreal.ExcavationTerrainChunkSubsystem)
    res['landscapes'] = len(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.LandscapeProxy))
    chunks = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ExcavationTerrainChunkActor)
    res['chunk_actors'] = len(chunks)
    res['streaming'] = [ts.get_editor_property(k) for k in ('enable_automatic_streaming', 'horizontal_chunk_radius', 'underground_chunk_layers', 'above_player_chunk_layers')] if ts else None
    res['cfg'] = [ex.get_editor_property('cell_size_cm'), ex.get_editor_property('cells_per_chunk_axis')]
    tris = []
    for c in chunks:
        dm = c.get_editor_property('dynamic_mesh_component').get_dynamic_mesh()
        tris.append([str(c.get_chunk_coordinate()), unreal.GeometryScript_MeshQueries.get_num_triangle_i_ds(dm) if hasattr(unreal, 'GeometryScript_MeshQueries') else -1])
    res['chunk_tris'] = sorted(set(t[1] for t in tris))
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    p = pawn.get_actor_location()
    # ground under the player
    hit = unreal.SystemLibrary.line_trace_single(world, p, p - unreal.Vector(0, 0, 5000), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [pawn], unreal.DrawDebugTrace.NONE, True)
    g = hit.to_tuple()[4] if hit else p
    res['ground'] = [round(g.x), round(g.y), round(g.z), str(hit.to_tuple()[9].get_name()) if hit and hit.to_tuple()[9] else None]
    res['density_below_ground'] = ex.try_sample_density_at_world(g - unreal.Vector(0, 0, 50))
    res['density_above_ground'] = ex.try_sample_density_at_world(g + unreal.Vector(0, 0, 100))
    edits = []
    for i in range(10):
        c = g + unreal.Vector(400 + i * 60, 0, -20)
        t0 = time.perf_counter()
        r = ex.apply_sphere_edit(c, 40.0, unreal.ExcavationEditType.CARVE)
        dt = (time.perf_counter() - t0) * 1000
        t1 = time.perf_counter(); nq = ts.flush_pending_rebuilds(); dq = (time.perf_counter() - t1) * 1000
        rec = r[1] if isinstance(r, tuple) else r
        ok = rec is not None and not isinstance(rec, bool)
        edits.append([round(dt, 2), nq, round(dq, 2), ok, len(rec.get_editor_property('changes')) if ok else str(r)])
    res['edits_r40'] = edits
    big = []
    for i in range(3):
        c = g + unreal.Vector(-800, i * 400, -100)
        t0 = time.perf_counter(); r = ex.apply_sphere_edit(c, 150.0, unreal.ExcavationEditType.CARVE); t1 = time.perf_counter(); nq = ts.flush_pending_rebuilds(); big.append([round((t1 - t0) * 1000, 2), nq, round((time.perf_counter() - t1) * 1000, 2)])
    res['edits_r150'] = big
    reb = []
    for c in [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ExcavationTerrainChunkActor) if a.get_editor_property('last_triangle_count') > 0][:6] + list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ExcavationTerrainChunkActor))[:2]:
        t0 = time.perf_counter(); c.rebuild_terrain_chunk(); reb.append([round((time.perf_counter() - t0) * 1000, 2)] + [round(c.get_editor_property(k), 2) for k in ('last_sample_ms', 'last_mesh_ms', 'last_upload_ms')] + [c.get_editor_property('last_triangle_count')])
    res['rebuild_ms'] = reb
    res['override_cells'] = ex.get_override_cell_count()
    res['chunk_actors_after'] = len(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ExcavationTerrainChunkActor))
json.dump(res, open(OUT, 'w'), indent=1, default=str)
