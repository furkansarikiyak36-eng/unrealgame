"""PIE: builds the showcase stations with the player's placement; report -> Saved/house_run.txt.
Stations come from Saved/probe_stations.txt (1-based, comma separated: "3,5,14"); without that file all of them are built."""
import unreal, os
saved = unreal.Paths.project_saved_dir()
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
shows = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.LogShowcase)
sel = os.path.join(saved, 'probe_stations.txt')
if not shows:
    out = 'no showcase'
elif os.path.exists(sel):
    ids = [int(x) for x in open(sel).read().replace('\n', ',').split(',') if x.strip()]
    out = '\n'.join(shows[0].build_station(i - 1) for i in ids)
else:
    out = shows[0].build_all()
open(os.path.join(saved, 'house_run.txt'), 'w', encoding='utf-8').write(out)
