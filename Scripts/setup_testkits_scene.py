"""
setup_testkits_scene.py  –  TestKits haritasını temizler ve düzenli grid ile yeniden kurar.

LAYOUT  (Y ekseni boyunca sıralar, X ekseninde kategoriler)
  Sıra 0  X = -200  : Log_01..Log_25    (25 kütük, Y ortada, 120 cm aralık)
  Sıra 1  X =    0  : Log_26..Log_50
  Sıra 2  X =  300  : Stone_01..Stone_25
  Sıra 3  X =  500  : Stone_26..Stone_50
  Sıra 4  X =  800  : Cobblestone_01..25
  Sıra 5  X = 1100  : StoneWall_01..25

PlayerStart  : X = -600, Y = 0, Z = -210  (kütüklere bakacak şekilde)
"""

import os, json
import unreal

LOG_MESH_PATH = "/Game/UnrealExport/SM_RawLog_300.SM_RawLog_300"

STONE_MESH_PATHS = [
    "/Game/Fab/Megascans/3D/Small_Stones_Pack_vcljbb1iw/Low/vcljbb1iw_tier_3/StaticMeshes/vcljbb1iw_LOD0_TIER3_001.vcljbb1iw_LOD0_TIER3_001",
    "/Game/Fab/Megascans/3D/Small_Stones_Pack_vcljbb1iw/Low/vcljbb1iw_tier_3/StaticMeshes/vcljbb1iw_LOD0_TIER3_002.vcljbb1iw_LOD0_TIER3_002",
    "/Game/Fab/Megascans/3D/Small_Stones_Pack_vcljbb1iw/Low/vcljbb1iw_tier_3/StaticMeshes/vcljbb1iw_LOD0_TIER3_003.vcljbb1iw_LOD0_TIER3_003",
    "/Game/Fab/Megascans/3D/Small_Stones_Pack_vcljbb1iw/Low/vcljbb1iw_tier_3/StaticMeshes/vcljbb1iw_LOD0_TIER3_004.vcljbb1iw_LOD0_TIER3_004",
    "/Game/Fab/Megascans/3D/Small_Stones_Pack_vcljbb1iw/Low/vcljbb1iw_tier_3/StaticMeshes/vcljbb1iw_LOD0_TIER3_005.vcljbb1iw_LOD0_TIER3_005",
    "/Game/Fab/Megascans/3D/Small_Stones_Pack_vcljbb1iw/Low/vcljbb1iw_tier_3/StaticMeshes/vcljbb1iw_LOD0_TIER3_006.vcljbb1iw_LOD0_TIER3_006",
    "/Game/Fab/Megascans/3D/Small_Stones_Pack_vdjkbfmiw/Low/vdjkbfmiw_tier_3/StaticMeshes/vdjkbfmiw_LOD0_TIER3_001.vdjkbfmiw_LOD0_TIER3_001",
    "/Game/Fab/Megascans/3D/Small_Stones_Pack_vdjkbfmiw/Low/vdjkbfmiw_tier_3/StaticMeshes/vdjkbfmiw_LOD0_TIER3_002.vdjkbfmiw_LOD0_TIER3_002",
]

COBBLE_MESH_PATHS = [
    "/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Gray_Pack_xksldamqx/Low/xksldamqx_tier_3/StaticMeshes/xksldamqx_LOD0_TIER3_000.xksldamqx_LOD0_TIER3_000",
    "/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Gray_Pack_xksldamqx/Low/xksldamqx_tier_3/StaticMeshes/xksldamqx_LOD0_TIER3_001.xksldamqx_LOD0_TIER3_001",
    "/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Gray_Pack_xksldamqx/Low/xksldamqx_tier_3/StaticMeshes/xksldamqx_LOD0_TIER3_002.xksldamqx_LOD0_TIER3_002",
    "/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Brown_Pack_xkqmdhaqx/Low/xkqmdhaqx_tier_3/StaticMeshes/xkqmdhaqx_LOD0_TIER3_000.xkqmdhaqx_LOD0_TIER3_000",
    "/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Brown_Pack_xkqmdhaqx/Low/xkqmdhaqx_tier_3/StaticMeshes/xkqmdhaqx_LOD0_TIER3_001.xkqmdhaqx_LOD0_TIER3_001",
    "/Game/Fab/Megascans/3D/Roadside_Construction_Cobblestone_Brown_Pack_xkqmdhaqx/Low/xkqmdhaqx_tier_3/StaticMeshes/xkqmdhaqx_LOD0_TIER3_002.xkqmdhaqx_LOD0_TIER3_002",
]

WALL_MESH_PATH = "/Game/Fab/Megascans/3D/Castle_Wall_Stone_sdAta/Low/sdAta_tier_3/StaticMeshes/sdAta_tier_3.sdAta_tier_3"

SPACING_Y = 120.0   # cm – obje arası Y mesafesi
Z_GROUND  = -293.0  # cm – zemin yüksekliği

# prefix, count, sütun başına kaç obje, X başlangıcı, X sıra aralığı, Z, yaw modu (-1=çeşitli)
GROUPS = [
    {"prefix": "Log",         "count": 50, "cols": 25, "x_start": -200.0, "x_gap": 200.0, "z": Z_GROUND,       "rot_yaw":  0},
    {"prefix": "Stone",       "count": 50, "cols": 25, "x_start":  300.0, "x_gap": 200.0, "z": Z_GROUND,       "rot_yaw": -1},
    {"prefix": "Cobblestone", "count": 25, "cols": 25, "x_start":  800.0, "x_gap":   0.0, "z": Z_GROUND,       "rot_yaw": -1},
    {"prefix": "StoneWall",   "count": 25, "cols": 25, "x_start": 1100.0, "x_gap":   0.0, "z": Z_GROUND + 3.0, "rot_yaw":  0},
]


def load_meshes(paths):
    return [m for m in (unreal.load_object(None, p) for p in paths) if m]


def spawn_grid(eas, group, meshes):
    prefix  = group["prefix"]
    count   = group["count"]
    cols    = group["cols"]
    x_start = group["x_start"]
    x_gap   = group["x_gap"]
    z       = group["z"]
    yaw_mode = group["rot_yaw"]

    # Y'yi sahne ortasına (Y=0) göre ortala
    y_offset = -((cols - 1) * SPACING_Y / 2.0)

    for i in range(count):
        row = i // cols
        col = i % cols
        x = x_start + row * x_gap
        y = y_offset + col * SPACING_Y
        yaw = float((i * 47) % 360) if yaw_mode == -1 else float(yaw_mode)

        actor = eas.spawn_actor_from_class(
            unreal.StaticMeshActor,
            unreal.Vector(x, y, z)
        )
        actor.set_actor_label(f"{prefix}_{i + 1:02d}")
        actor.set_actor_rotation(unreal.Rotator(0.0, yaw, 0.0), False)

        if meshes:
            sm_comp = actor.static_mesh_component
            if sm_comp:
                sm_comp.set_static_mesh(meshes[i % len(meshes)])

    print(f"  {prefix}: {count} adet spawn edildi.")


def run():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    print("Loading /Game/Maps/TestKitsV2...")
    les.load_level("/Game/Maps/TestKitsV2")

    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # 1. Eski aktörleri listele ve kaydet
    actors = eas.get_all_level_actors()
    dump_lines = []
    for a in actors:
        if a:
            loc = a.get_actor_location()
            dump_lines.append(f"{a.get_actor_label()} ({a.get_class().get_name()}) at ({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f})")
    saved_actors_path = os.path.join(unreal.Paths.project_saved_dir(), "all_testkits_actors.txt")
    with open(saved_actors_path, "w", encoding="utf-8") as f:
        f.write("\n".join(dump_lines))
    print(f"Dumped {len(dump_lines)} actors to {saved_actors_path}")

    env_labels = {"DirectionalLight", "SkyAtmosphere", "SkyLight", "ExponentialHeightFog", "VolumetricCloud", "SM_SkySphere", "Floor", "PlayerStart"}
    cleared = 0
    for a in actors:
        if not a:
            continue
        label = a.get_actor_label()
        if label in env_labels or label.startswith("Floor") or label.startswith("SM_SkySphere") or isinstance(a, unreal.PlayerStart):
            continue
        eas.destroy_actor(a)
        cleared += 1
    print(f"Temizlendi: {cleared} aktor.")


    # 2. Mesh'leri yükle
    mesh_map = {
        "Log":         load_meshes([LOG_MESH_PATH]),
        "Stone":       load_meshes(STONE_MESH_PATHS),
        "Cobblestone": load_meshes(COBBLE_MESH_PATHS),
        "StoneWall":   load_meshes([WALL_MESH_PATH]),
    }
    for k, v in mesh_map.items():
        print(f"  Mesh '{k}': {len(v)} yüklendi.")

    # 3. Spawn
    for grp in GROUPS:
        spawn_grid(eas, grp, mesh_map[grp["prefix"]])

    # 4. PlayerStart konumlandır & Floor etiketini ayarla
    for a in eas.get_all_level_actors():
        if isinstance(a, unreal.PlayerStart):
            a.set_actor_location(unreal.Vector(-600.0, 0.0, -210.0), False, False)
            a.set_actor_rotation(unreal.Rotator(0.0, 0.0, 0.0), False)
            print("PlayerStart: (-600, 0, -210) yonü +X")
        elif a and a.get_actor_label() == "Floor":
            tags = [str(t) for t in a.tags]
            if "NoDiggableGround" not in tags:
                a.tags.append("NoDiggableGround")
            if hasattr(a, "static_mesh_component") and a.static_mesh_component:
                a.static_mesh_component.set_collision_profile_name("BlockAll")
            print("Floor: NoDiggableGround etiketi ve BlockAll profili ayarlandi.")

    # 5. Kaydet (yeni API)
    try:
        lvl_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        lvl_sub.save_current_level()
        print("Level kaydedildi (LevelEditorSubsystem).")
    except Exception as e:
        unreal.EditorLevelLibrary.save_current_level()
        print(f"Level kaydedildi (EditorLevelLibrary). ({e})")

    result = {"logs": 50, "stones": 50, "cobblestones": 25, "stonewalls": 25, "cleared": cleared}
    saved_path = os.path.join(unreal.Paths.project_saved_dir(), "setup_testkits.json")
    with open(saved_path, "w", encoding="utf-8") as f:
        json.dump(result, f, indent=2)
    print(f"Bitti: {result}")


run()

