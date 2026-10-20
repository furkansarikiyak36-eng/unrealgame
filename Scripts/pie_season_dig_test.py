"""PIE test of ChatGPT's season / digging / layered water-soil mechanics: does it work, and what does it cost.
py ".../pie_season_dig_test.py" [quick] -> Saved/pie_season_dig_test.json
Needs a running PIE session (ForestCharacter in the world)."""
import json, os, sys, time
import unreal

OUT = os.path.join(unreal.Paths.project_saved_dir(), 'pie_season_dig_test.json')
quick = 'quick' in sys.argv
B = unreal.SeasonTerrainBench
res = {'log': []}
def log(k, v):
    res['log'].append([k, v if isinstance(v, (str, int, float, bool, list, dict)) or v is None else str(v)])

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if not world:
    log('error', 'no PIE world')
else:
    def sub(cls):
        for o in unreal.ObjectIterator(cls):
            if o.get_outer() == world: return o
        return None
    sim = sub(unreal.SimulationSubsystem)
    dig = sub(unreal.DiggableTerrainSubsystem)
    ws = sub(unreal.WorldStateSubsystem)
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    here = pawn.get_actor_location() if pawn else unreal.Vector(0, 0, 0)
    log('player', [round(here.x), round(here.y), round(here.z)])
    log('chunk', ws.get_editor_property('chunk_settings').export_text())

    # 1. Seasons: walk a whole year with the game clock, sample the calendar at each season edge.
    log('season_start', B.describe_season_at_world(world, here))
    t0 = time.perf_counter()
    days = 0
    for target in (89, 90, 91, 180, 181, 270, 271, 360, 361):
        state = sim.get_state()
        cur = state.get_editor_property('day_number') if state else 0
        if target > cur:
            sim.advance_days(target - cur); days += target - cur
        log('day%d' % target, B.describe_season_at_world(world, here))
    log('year_advance_ms', round((time.perf_counter() - t0) * 1000, 1))
    log('year_days', days)

    # 2. Digging at the player's feet: dig until it refuses, fill back, till, ask if planting is possible.
    spot = here + unreal.Vector(300, 0, 0)
    log('cell_before', B.describe_cell_at_world(world, spot))
    for i in range(6):
        r = dig.dig_at_world(spot, 1.0, 0.5, unreal.DigBrushShape.CIRCLE)
        log('dig%d' % i, r.export_text())
        log('cell_dig%d' % i, B.describe_cell_at_world(world, spot))
    r = dig.fill_at_world(spot, 1.0, 50.0, unreal.DigBrushShape.CIRCLE)
    log('fill', r.export_text())
    log('cell_fill', B.describe_cell_at_world(world, spot))
    r = dig.prepare_soil_for_planting(spot, 1.0, unreal.DigBrushShape.CIRCLE)
    log('till', r.export_text())
    log('cell_till', B.describe_cell_at_world(world, spot))
    log('can_plant', repr(dig.can_plant_at_world(spot, 0.4)))
    # big shovel vs. one cell
    log('dig_r0', dig.dig_at_world(spot + unreal.Vector(2000, 0, 0), 0.1, 0.3, unreal.DigBrushShape.GRID_CELL).export_text())
    log('dig_r6', dig.dig_at_world(spot + unreal.Vector(4000, 0, 0), 6.0, 0.3, unreal.DigBrushShape.CIRCLE).export_text())

    # 3. Cost.
    for n in ((16, 64) if quick else (16, 64, 256, 1024)):
        log('hydro_%d' % n, B.benchmark_hydrology(n, 30, 12.0, 8.0))
    log('hydro_snow_64', B.benchmark_hydrology(64, 30, -6.0, 8.0))
    for n, s in (((16, 5),) if quick else ((16, 10), (64, 10), (256, 5))):
        log('world_%d' % n, B.benchmark_world_step(world, n, s))
    log('dig_r1', B.benchmark_digging(world, 200 if quick else 1000, 1.0))
    log('dig_r6b', B.benchmark_digging(world, 50 if quick else 200, 6.0))
    log('cell_after_bench', B.describe_cell_at_world(world, spot))

json.dump(res, open(OUT, 'w'), indent=1)
