"""PIE: dump the world cell and season at the player (+3 m) -> Saved/pie_cell_probe.json"""
import json, os
import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
out = {}
if world:
    p = unreal.GameplayStatics.get_player_pawn(world, 0).get_actor_location() + unreal.Vector(300, 0, 0)
    out['cell'] = unreal.SeasonTerrainBench.describe_cell_at_world(world, p)
    out['season'] = unreal.SeasonTerrainBench.describe_season_at_world(world, p)
    for o in unreal.ObjectIterator(unreal.WorldStateSubsystem):
        if o.get_outer() == world:
            out['worldtime'] = o.get_world_time().export_text()
            out['weather'] = [o.get_editor_property(k) for k in ('temperature_c', 'rainfall_mm', 'is_raining', 'is_snowing')]
            c = o.get_or_create_chunk_at_world(p)
            out['chunk_meta'] = c.get_editor_property('metadata').export_text()
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'pie_cell_probe.json'), 'w'), indent=1)
