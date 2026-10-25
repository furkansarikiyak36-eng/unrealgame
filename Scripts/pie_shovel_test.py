"""PIE: shovel straight down at three spots until rock stops it, empty the sack, fill a hole back, and count how much
of the soil layer is boulder. py ".../pie_shovel_test.py" -> Saved/pie_shovel_test.json"""
import json, os
import unreal
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'pie_shovel_test.json')
res = {}
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
def sub(cls):
    for o in unreal.ObjectIterator(cls):
        if o.get_outer() == world: return o
if world:
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    ex = sub(unreal.ExcavationVolumeSubsystem)
    ts = sub(unreal.ExcavationTerrainChunkSubsystem)
    base = pawn.get_actor_location()
    up = unreal.Vector(0, 0, 1)
    for n, off in enumerate((unreal.Vector(500, 0, 0), unreal.Vector(0, 700, 0), unreal.Vector(-900, -300, 0))):
        log = []
        x = base + off
        for k in range(14):
            p = unreal.Vector(x.x, x.y, -k * 40.0)
            msg = pawn.test_shovel(p, up, False)
            log.append([k, round(p.z), msg, round(pawn.test_loose_soil(), 2)])
            if 'Kaya' in msg: break
            if 'dolu' in msg:   # sack full: dump it away from the hole
                for j in range(4):
                    pawn.test_shovel(base + unreal.Vector(-400 - 60 * j, 600, 0), up, True)
        res['spot%d' % n] = log
    # fill the first hole back
    x = base + unreal.Vector(500, 0, 0)
    res['fill'] = [pawn.test_shovel(unreal.Vector(x.x, x.y, -k * 40.0 - 40), up, True) for k in range(3)]
    ts.flush_pending_rebuilds()
    # how much of the soil (0..-2 m) is boulder rock
    rock = tot = 0
    for i in range(30):
        for j in range(30):
            for z in (-60, -110, -160):
                m = ex.try_sample_base_material_id_at_world(unreal.Vector(base.x + i * 50, base.y + j * 50, z))
                tot += 1; rock += 1 if m == 2 else 0
    res['boulder_fraction_0_2m'] = round(rock / tot, 3)
    res['bedrock_depths_m'] = []
    for i in range(8):
        for z in range(0, -500, -10):
            if ex.try_sample_base_material_id_at_world(unreal.Vector(base.x + i * 3000, base.y, z)) == 2 and \
               ex.try_sample_base_material_id_at_world(unreal.Vector(base.x + i * 3000, base.y, z - 50)) == 2:
                res['bedrock_depths_m'].append(-z / 100.0); break
json.dump(res, open(OUT, 'w'), indent=1, ensure_ascii=False)
