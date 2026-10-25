"""PIE: build a log wall with the character's own placement (TestBuildWall): a base log, N courses stacked on it while
facing it, and one crossing log over its end while looking along it. py ".../pie_wall_test.py" [length] [courses]
-> Saved/pie_wall_test.json"""
import json, os, sys
import unreal
args = [a for a in sys.argv[1:] if a.isdigit()]
length = int(args[0]) if args else 300
courses = int(args[1]) if len(args) > 1 else 4
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
out = {}
if world:
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    out['report'] = pawn.test_build_wall(length, courses)
    out['last_message'] = pawn.test_last_message()
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'pie_wall_test.json'), 'w'), indent=1, ensure_ascii=False)
