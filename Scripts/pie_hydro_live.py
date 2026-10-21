"""PIE: one sample of the live world cell near the player (season, weather, soil water layers, snow), then moves the
game clock on by N days (argument, default 30). Run it repeatedly to walk through a year; samples are appended to
Saved/pie_hydro_year.json ("reset" as first argument starts a new file)."""
import json, os, sys
import unreal
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'pie_hydro_year.json')
DONE = os.path.join(unreal.Paths.project_saved_dir(), 'pie_hydro_live.json')
args = [a for a in sys.argv[1:]]
data = [] if ('reset' in args or not os.path.exists(OUT)) else json.load(open(OUT))
n = int(next((a for a in args if a.isdigit()), '30'))
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if world:
    B = unreal.SeasonTerrainBench
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    p = pawn.get_actor_location() + unreal.Vector(-600, 600, 0)
    s = {'season': B.describe_season_at_world(world, p), 'cell': B.describe_cell_at_world(world, p)}
    for o in unreal.ObjectIterator(unreal.WorldStateSubsystem):
        if o.get_outer() == world:
            s['weather'] = [round(o.get_editor_property(k), 2) if isinstance(o.get_editor_property(k), float) else o.get_editor_property(k)
                            for k in ('temp_min_c', 'temp_max_c', 'rainfall_mm', 'is_snowing')]
            s['meta'] = o.get_or_create_chunk_at_world(p).get_editor_property('metadata').export_text()
    for o in unreal.ObjectIterator(unreal.WorldChunkSubsystem):
        if o.get_outer() == world: s['loaded_chunks'] = len(o.get_editor_property('loaded_chunks'))
    data.append(s)
    for o in unreal.ObjectIterator(unreal.SimulationSubsystem):
        if o.get_outer() == world: o.advance_days(n)
json.dump(data, open(OUT, 'w'), indent=1)
json.dump({'samples': len(data)}, open(DONE, 'w'))
