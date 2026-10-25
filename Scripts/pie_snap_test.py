"""PIE: snap sectors on a log (TestSnapSectors): which placement each view angle picks, then +45 / 4 courses / square
across built on it. py ".../pie_snap_test.py" [length] -> Saved/pie_snap_test.json"""
import json, os, sys
import unreal
length = int(sys.argv[1]) if len(sys.argv) > 1 and sys.argv[1].isdigit() else 300
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
out = {}
if world:
    out['report'] = unreal.GameplayStatics.get_player_pawn(world, 0).test_snap_sectors(length)
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'pie_snap_test.json'), 'w'), indent=1, ensure_ascii=False)
