"""PIE: puts the player before station N of the LogShowcase map (stations 14 m apart along +X from the origin) looking
at it, for a screenshot. py ".../pie_showcase_shot.py" N -> Saved/pie_showcase_shot.json"""
import json, math, os, sys
import unreal
n = int(sys.argv[1]) if len(sys.argv) > 1 else 1
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
pc = unreal.GameplayStatics.get_player_controller(world, 0)
target = unreal.Vector((n - 1) * 1400 + 100, 120, 120)
eye = unreal.Vector(target.x - 650, target.y - 650, 380)
pawn.set_actor_location(eye, False, True)
d = target - eye
yaw = math.degrees(math.atan2(d.y, d.x)); pitch = math.degrees(math.atan2(d.z, math.hypot(d.x, d.y)))
pc.set_control_rotation(unreal.Rotator(roll=0, pitch=pitch, yaw=yaw))
json.dump({'n': n}, open(os.path.join(unreal.Paths.project_saved_dir(), 'pie_showcase_shot.json'), 'w'))
