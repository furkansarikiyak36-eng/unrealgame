"""
Builds the greybox content the gameplay code expects (see ForestTypes.cpp) and the Greybox_Mechanics test map.

Run with the editor closed:
  UnrealEditor-Cmd.exe MyProject2.uproject -run=pythonscript -script=<path>/build_greybox.py -unattended -nosplash

Conventions (the C++ relies on them):
  - Logs, planks and beams lie along +X with the pivot at their centre, modelled at real size (cm).
  - Material slot 0 is the outside (bark / stone / handle), slot 1 is cut wood or metal.
  - Tools stand along +Z from the handle end (z = 0), head on top, edge toward +X.
  - Trees and rocks have their pivot on the ground; only the trunk collides.
  - The opening frame is 100 x 40 x 100 cm standing on its bottom edge (scaled to the opening in game).
Re-running replaces everything it made.

Set GREYBOX_BUILD=demo to rebuild only the Demo_WoodChain map (meshes and Greybox_Mechanics are left alone).
"""
import math
import random

import unreal

GB = '/Game/Greybox'
MESHES = GB + '/Meshes'
MATS = GB + '/Materials'
MAP_PATH = '/Game/Maps/Greybox_Mechanics'
DEMO_MAP_PATH = '/Game/Maps/Demo_WoodChain'
CONTEXTUAL_DEMO_MAP_PATH = '/Game/Maps/Demo_Contextual_01'

asset_lib = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mat_lib = unreal.MaterialEditingLibrary
prim = unreal.GeometryScript_Primitives
booleans = unreal.GeometryScript_MeshBooleans
gs_mats = unreal.GeometryScript_Materials
selection = unreal.GeometryScript_MeshSelection
transforms = unreal.GeometryScript_MeshTransforms
edits = unreal.GeometryScript_MeshEdits
normals = unreal.GeometryScript_Normals
queries = unreal.GeometryScript_MeshQueries
collision = unreal.GeometryScript_Collision
new_assets = unreal.GeometryScript_NewAssetUtils

LOG = []


def log(*parts):
    line = ' '.join(str(p) for p in parts)
    LOG.append(line)
    unreal.log('[Greybox] ' + line)


# ---------------------------------------------------------------------------
# Materials
# ---------------------------------------------------------------------------

def make_colour_material(name, colour):
    """Instance of the engine's basic shape material, tinted through its Color parameter."""
    path = MATS + '/' + name
    if asset_lib.does_asset_exist(path):
        asset_lib.delete_asset(path)
    mi = asset_tools.create_asset(name, MATS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mat_lib.set_material_instance_parent(mi, unreal.load_asset('/Engine/BasicShapes/BasicShapeMaterial'))
    mat_lib.set_material_instance_vector_parameter_value(mi, 'Color', unreal.LinearColor(*colour, 1.0))
    asset_lib.save_loaded_asset(mi)
    return mi


# ---------------------------------------------------------------------------
# Mesh helpers
# ---------------------------------------------------------------------------

def opts(material_id=0):
    o = unreal.GeometryScriptPrimitiveOptions()
    o.material_id = material_id
    return o


def xf(loc=(0, 0, 0), rot=(0, 0, 0), scale=(1, 1, 1)):
    """rot is (pitch, yaw, roll) in degrees."""
    return unreal.Transform(
        location=unreal.Vector(*loc),
        rotation=unreal.Rotator(roll=rot[2], pitch=rot[0], yaw=rot[1]),
        scale=unreal.Vector(*scale))


def new_mesh():
    return unreal.DynamicMesh()


# A cylinder from append_cylinder runs along +Z; pitch 90 turns it onto the X axis.
ALONG_X = (90, 0, 0)


def append_log(mesh, length, diameter, steps=24, cap_material=1, at=(0, 0, 0)):
    prim.append_cylinder(mesh, opts(0), xf(at, ALONG_X), diameter * 0.5, length, steps, 0, True,
                         unreal.GeometryScriptPrimitiveOriginMode.CENTER)
    paint_by_normal(mesh, (1, 0, 0), 15, cap_material)
    paint_by_normal(mesh, (-1, 0, 0), 15, cap_material)


def paint_by_normal(mesh, normal, max_angle, material_id):
    _, sel = selection.select_mesh_elements_by_normal_angle(
        mesh, unreal.Vector(*normal), max_angle, unreal.GeometryScriptMeshSelectionType.TRIANGLES, False, 3)
    gs_mats.set_material_id_for_mesh_selection(mesh, sel, material_id)


def subtract(target, tool):
    o = unreal.GeometryScriptMeshBooleanOptions()
    o.fill_holes = True
    o.simplify_output = False
    booleans.apply_mesh_boolean(target, unreal.Transform(), tool, unreal.Transform(),
                                unreal.GeometryScriptBooleanOperation.SUBTRACT, o)


def bounds(mesh):
    return queries.get_mesh_bounding_box(mesh)


def recentre(mesh):
    b = bounds(mesh)
    c = (b.min + b.max) * 0.5
    transforms.translate_mesh(mesh, unreal.Vector(-c.x, -c.y, -c.z))


def hard_edges(mesh, angle=40.0):
    split = unreal.GeometryScriptSplitNormalsOptions()
    split.split_by_opening_angle = True
    split.opening_angle_deg = angle
    normals.compute_split_normals(mesh, split, unreal.GeometryScriptCalculateNormalsOptions())


def save_mesh(name, mesh, materials, collision_mesh=None, hulls=1, collide=True):
    """Writes the dynamic mesh to a static mesh asset with the given slot materials and simple collision."""
    path = MESHES + '/' + name
    if asset_lib.does_asset_exist(path):
        asset_lib.delete_asset(path)
    hard_edges(mesh)
    o = unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
    o.enable_nanite = False
    o.enable_collision = False
    o.enable_recompute_normals = False
    o.enable_recompute_tangents = True
    asset, outcome = new_assets.create_new_static_mesh_asset_from_mesh(mesh, path, o)
    if asset is None:
        log('FAILED to create', path, outcome)
        return None

    slots = list(asset.get_editor_property('static_materials'))
    while len(slots) < len(materials):
        slots.append(unreal.StaticMaterial())
    for index, material in enumerate(materials):
        slots[index].set_editor_property('material_interface', material)
        slots[index].set_editor_property('material_slot_name', 'Slot%d' % index)
    asset.set_editor_property('static_materials', slots)

    if collide:
        c = unreal.GeometryScriptCollisionFromMeshOptions()
        c.emit_transaction = False
        c.method = unreal.GeometryScriptCollisionGenerationMethod.CONVEX_HULLS
        c.max_convex_hulls_per_mesh = hulls
        collision.set_static_mesh_collision_from_mesh(collision_mesh or mesh, asset, c)
        body = asset.get_editor_property('body_setup')
        if body:
            body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)

    asset_lib.save_loaded_asset(asset)
    b = asset.get_bounding_box()
    shapes = collision.get_simple_collision_shape_count(collision.get_simple_collision_from_static_mesh(asset)) if collide else 0
    log('mesh', name, 'size', round(b.max.x - b.min.x), round(b.max.y - b.min.y), round(b.max.z - b.min.z),
        'slots', len(slots), 'collision shapes', shapes)
    return asset


# ---------------------------------------------------------------------------
# Meshes
# ---------------------------------------------------------------------------

def build_meshes(m):
    bark, cut, stone, leaves, pine, handle, metal = (
        m['bark'], m['cut'], m['stone'], m['leaves'], m['pine'], m['handle'], m['metal'])
    made = {}

    # Carried log (stage 3). The trunk and big log stages are this mesh stretched in code.
    LOG_LEN, LOG_D = 390.0, 70.0
    mesh = new_mesh()
    append_log(mesh, LOG_LEN, LOG_D)
    made['Log'] = save_mesh('SM_GB_Log', mesh, [bark, cut])

    mesh = new_mesh()
    append_log(mesh, 120.0, LOG_D)
    made['LogShort'] = save_mesh('SM_GB_LogShort', mesh, [bark, cut])

    # Split lengthwise, flat face up.
    mesh = new_mesh()
    append_log(mesh, LOG_LEN, LOG_D)
    cut_opts = unreal.GeometryScriptMeshPlaneCutOptions()
    cut_opts.fill_holes = True
    cut_opts.hole_fill_material_id = 1
    booleans.apply_mesh_plane_cut(mesh, unreal.Transform(), cut_opts)
    if bounds(mesh).max.z > 1.0:
        mesh = new_mesh()
        append_log(mesh, LOG_LEN, LOG_D)
        cut_opts.flip_cut_side = True
        booleans.apply_mesh_plane_cut(mesh, unreal.Transform(), cut_opts)
    recentre(mesh)
    made['LogHalf'] = save_mesh('SM_GB_LogHalf', mesh, [bark, cut])

    # A trough: a long box carved out of the top.
    mesh = new_mesh()
    append_log(mesh, LOG_LEN, LOG_D)
    tool = new_mesh()
    prim.append_box(tool, opts(1), xf((0, 0, -LOG_D * 0.1)), LOG_LEN - 50.0, LOG_D * 0.7, LOG_D, 0, 0, 0,
                    unreal.GeometryScriptPrimitiveOriginMode.BASE)
    subtract(mesh, tool)
    made['LogHollow'] = save_mesh('SM_GB_LogHollow', mesh, [bark, cut])

    mesh = new_mesh()
    prim.append_box(mesh, opts(0), xf(), LOG_LEN, 35.0, 10.0, 0, 0, 0, unreal.GeometryScriptPrimitiveOriginMode.CENTER)
    made['Plank'] = save_mesh('SM_GB_Plank', mesh, [cut])

    mesh = new_mesh()
    prim.append_box(mesh, opts(0), xf(), 400.0, 25.0, 25.0, 0, 0, 0, unreal.GeometryScriptPrimitiveOriginMode.CENTER)
    made['Beam'] = save_mesh('SM_GB_Beam', mesh, [cut])

    mesh = new_mesh()
    append_log(mesh, 45.0, 14.0, steps=10)
    made['Firewood'] = save_mesh('SM_GB_Firewood', mesh, [bark, cut])

    # A separate small physical stick; the countable item is not a log cutout.
    mesh = new_mesh()
    append_log(mesh, 55.0, 2.8, steps=8)
    made['Stick'] = save_mesh('SM_GB_Stick', mesh, [bark, cut])

    # Wood chips thrown off by axe hits.
    mesh = new_mesh()
    prim.append_box(mesh, opts(0), xf(), 12.0, 6.0, 2.5, 0, 0, 0, unreal.GeometryScriptPrimitiveOriginMode.CENTER)
    made['Chip'] = save_mesh('SM_GB_Chip', mesh, [cut])

    # Faceted pebble.
    mesh = new_mesh()
    prim.append_sphere_lat_long(mesh, opts(0), xf(scale=(1.25, 1.0, 0.7)), 13.0, 4, 6,
                                unreal.GeometryScriptPrimitiveOriginMode.CENTER)
    made['Stone'] = save_mesh('SM_GB_Stone', mesh, [stone])

    # Dressed block, standing on its base.
    mesh = new_mesh()
    prim.append_box(mesh, opts(0), xf(), 60.0, 40.0, 30.0, 0, 0, 0, unreal.GeometryScriptPrimitiveOriginMode.BASE)
    made['StoneBlock'] = save_mesh('SM_GB_StoneBlock', mesh, [stone])

    # Door / window frame: 100 x 100 with 8 cm boards, 40 cm deep, on its bottom edge.
    mesh = new_mesh()
    base = unreal.GeometryScriptPrimitiveOriginMode.BASE
    prim.append_box(mesh, opts(0), xf((-46, 0, 0)), 8.0, 40.0, 100.0, 0, 0, 0, base)
    prim.append_box(mesh, opts(0), xf((46, 0, 0)), 8.0, 40.0, 100.0, 0, 0, 0, base)
    prim.append_box(mesh, opts(0), xf((0, 0, 92)), 84.0, 40.0, 8.0, 0, 0, 0, base)
    prim.append_box(mesh, opts(0), xf((0, 0, 0)), 84.0, 40.0, 8.0, 0, 0, 0, base)
    made['Frame'] = save_mesh('SM_GB_Frame', mesh, [cut], hulls=4)

    # Raw logs for in-game carving (K): thinner, longer, finer round.
    for length in (500, 375, 250):
        mesh = new_mesh()
        append_log(mesh, float(length), 32.0, steps=32)
        made['RawLog_%d' % length] = save_mesh('SM_GB_RawLog_%d' % length, mesh, [bark, cut], hulls=1)

    # Tools: handle along +Z from z = 0, head on top, edge toward +X.
    mesh = new_mesh()
    prim.append_cylinder(mesh, opts(0), xf(), 1.8, 55.0, 10, 0, True, base)
    prim.append_box(mesh, opts(1), xf((4, 0, 45)), 16.0, 3.0, 12.0, 0, 0, 0, base)
    prim.append_box(mesh, opts(1), xf((-6, 0, 47)), 5.0, 4.0, 8.0, 0, 0, 0, base)
    made['Axe'] = save_mesh('SM_GB_Tool_Axe', mesh, [handle, metal], collide=False)

    mesh = new_mesh()
    prim.append_cylinder(mesh, opts(0), xf(), 2.0, 66.0, 10, 0, True, base)
    prim.append_box(mesh, opts(1), xf((0, 0, 58)), 46.0, 4.0, 5.0, 0, 0, 0, base)
    # Pitch -90 turns a cone's +Z onto +X, pitch 90 onto -X: both picks point outward.
    prim.append_cone(mesh, opts(1), xf((23, 0, 60.5), (-90, 0, 0)), 2.5, 0.3, 8.0, 8, 0, True, base)
    prim.append_cone(mesh, opts(1), xf((-23, 0, 60.5), (90, 0, 0)), 2.5, 0.3, 8.0, 8, 0, True, base)
    made['Pickaxe'] = save_mesh('SM_GB_Tool_Pickaxe', mesh, [handle, metal], collide=False)

    mesh = new_mesh()
    prim.append_cylinder(mesh, opts(0), xf(), 1.6, 12.0, 10, 0, True, base)
    prim.append_box(mesh, opts(1), xf((0, 0, 12)), 1.0, 1.0, 4.0, 0, 0, 0, base)
    prim.append_box(mesh, opts(1), xf((0, 0, 16)), 0.5, 8.0, 24.0, 0, 0, 0, base)
    made['Trowel'] = save_mesh('SM_GB_Tool_Trowel', mesh, [handle, metal], collide=False)

    # Trees: pivot at the root, trunk is the only collision. Height decides the hits to fell:
    # over 15 m takes 8 axe hits, the pine (11 m) takes 6.
    mesh = new_mesh()
    trunk = new_mesh()
    prim.append_cylinder(trunk, opts(0), xf(), 32.0, 1600.0, 16, 0, True, base)
    edits.append_mesh(mesh, trunk, unreal.Transform())
    prim.append_sphere_lat_long(mesh, opts(1), xf((0, 0, 1150), scale=(1.0, 1.0, 0.85)), 450.0, 6, 10,
                                unreal.GeometryScriptPrimitiveOriginMode.CENTER)
    prim.append_sphere_lat_long(mesh, opts(1), xf((180, 120, 950), scale=(1.0, 1.0, 0.8)), 300.0, 5, 8,
                                unreal.GeometryScriptPrimitiveOriginMode.CENTER)
    prim.append_sphere_lat_long(mesh, opts(1), xf((-160, -140, 1000), scale=(1.0, 1.0, 0.8)), 280.0, 5, 8,
                                unreal.GeometryScriptPrimitiveOriginMode.CENTER)
    made['TreeBroad'] = save_mesh('SM_GB_Tree_Broad', mesh, [bark, leaves], collision_mesh=trunk)

    mesh = new_mesh()
    trunk = new_mesh()
    prim.append_cylinder(trunk, opts(0), xf(), 22.0, 1100.0, 12, 0, True, base)
    edits.append_mesh(mesh, trunk, unreal.Transform())
    prim.append_cone(mesh, opts(1), xf((0, 0, 300)), 240.0, 20.0, 480.0, 10, 0, True, base)
    prim.append_cone(mesh, opts(1), xf((0, 0, 620)), 180.0, 0.0, 480.0, 10, 0, True, base)
    made['TreePine'] = save_mesh('SM_GB_Tree_Pine', mesh, [bark, pine], collision_mesh=trunk)

    # Boulder: faceted, sunk a little into the ground, pivot on the ground.
    mesh = new_mesh()
    prim.append_sphere_lat_long(mesh, opts(0), xf((0, 0, 38), scale=(1.3, 1.0, 0.75)), 70.0, 5, 7,
                                unreal.GeometryScriptPrimitiveOriginMode.CENTER)
    made['Rock'] = save_mesh('SM_GB_Rock', mesh, [stone])

    return made


# ---------------------------------------------------------------------------
# Map
# ---------------------------------------------------------------------------

def spawn(cls, loc, rot=(0, 0, 0), label=None, folder=None):
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(roll=rot[2], pitch=rot[0], yaw=rot[1]))
    if actor and label:
        actor.set_actor_label(label)
    if actor and folder:
        actor.set_folder_path(folder)
    return actor


def spawn_mesh(static_mesh, loc, rot=(0, 0, 0), scale=(1, 1, 1), label=None, folder=None, material=None):
    actor = spawn(unreal.StaticMeshActor, loc, rot, label, folder)
    comp = actor.static_mesh_component
    comp.set_static_mesh(static_mesh)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    if material:
        comp.set_material(0, material)
    return actor


def sign(text, loc, face_towards=(0, 0), size=70.0, colour=(255, 240, 200)):
    yaw = math.degrees(math.atan2(face_towards[1] - loc[1], face_towards[0] - loc[0]))
    actor = spawn(unreal.TextRenderActor, loc, (0, yaw, 0), 'Sign_' + text.split()[0], 'Signs')
    comp = actor.get_editor_property('text_render')
    comp.set_editor_property('text', text)
    comp.set_editor_property('world_size', size)
    comp.set_editor_property('horizontal_alignment', unreal.HorizTextAligment.EHTA_CENTER)
    comp.set_editor_property('text_render_color', unreal.Color(r=colour[0], g=colour[1], b=colour[2], a=255))
    return actor


def build_map(made, m):
    world = unreal.EditorLoadingAndSavingUtils.new_map_from_template('/Engine/Maps/Templates/Template_Default', False)
    if world is None:
        world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
        spawn(unreal.DirectionalLight, (0, 0, 1000), (-45, 30, 0), 'Sun')
        spawn(unreal.SkyLight, (0, 0, 500), label='SkyLight')
        spawn(unreal.SkyAtmosphere, (0, 0, 0), label='SkyAtmosphere')
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # The template's small floor goes; keep its sun, sky and fog.
    has_start = False
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.StaticMeshActor):
            actors.destroy_actor(actor)
        elif isinstance(actor, unreal.PlayerStart):
            actor.set_actor_location(unreal.Vector(0, 0, 100), False, False)
            actor.set_actor_rotation(unreal.Rotator(0, 0, 0), False)
            has_start = True
    if not has_start:
        spawn(unreal.PlayerStart, (0, 0, 100), label='PlayerStart')

    # 160 m ground slab with the engine's world grid: top face at z = 0.
    cube = unreal.load_asset('/Engine/BasicShapes/Cube')
    grid = unreal.load_asset('/Engine/EngineMaterials/WorldGridMaterial')
    spawn_mesh(cube, (0, 0, -50), scale=(160, 160, 1), label='Ground', folder='Ground', material=grid)

    rng = random.Random(7)

    # Forest on the left (-Y). Two trees near the start for quick testing.
    spawn_mesh(made['TreePine'], (600, -900, 0), (0, 20, 0), label='Tree_Pine_Test', folder='Forest')
    spawn_mesh(made['TreeBroad'], (1100, -1300, 0), (0, 70, 0), label='Tree_Broad_Test', folder='Forest')
    count = 0
    for gx in range(-3, 9):
        for gy in range(0, 6):
            if rng.random() < 0.3:
                continue
            x = gx * 600 + rng.uniform(-180, 180)
            y = -1900 - gy * 600 + rng.uniform(-180, 180)
            broad = rng.random() < 0.45
            scale = rng.uniform(1.0, 1.08) if broad else rng.uniform(0.85, 1.1)
            spawn_mesh(made['TreeBroad'] if broad else made['TreePine'], (x, y, 0), (0, rng.uniform(0, 360), 0),
                       (scale, scale, scale), label='Tree_%02d' % count, folder='Forest')
            count += 1
    log('trees', count + 2)

    # Rocks on the right (+Y). Two big boulders stay as scenery (scale above MaxRockScale = 2).
    spawn_mesh(made['Rock'], (700, 1000, 0), (0, 30, 0), label='Rock_Test', folder='Rocks')
    for index in range(14):
        x = rng.uniform(-1200, 3500)
        y = rng.uniform(1700, 4200)
        s = rng.uniform(0.7, 1.6)
        spawn_mesh(made['Rock'], (x, y, 0), (0, rng.uniform(0, 360), 0), (s, s * rng.uniform(0.8, 1.1), s),
                   label='Rock_%02d' % index, folder='Rocks')
    spawn_mesh(made['Rock'], (2600, 4800, 0), (0, 10, 0), (4, 3.4, 3), label='Boulder_Scenery_A', folder='Rocks')
    spawn_mesh(made['Rock'], (-600, 4500, 0), (0, 80, 0), (3.2, 3, 2.6), label='Boulder_Scenery_B', folder='Rocks')

    # Carving bench behind-right of the start: raw logs on two stone blocks each.
    carvable = unreal.load_class(None, '/Script/MyProject2.CarvableLog')
    for index, length in enumerate((500, 375, 250)):
        y = 500 + index * 160
        x = -900
        for side in (-1, 1):
            spawn_mesh(made['StoneBlock'], (x + side * length * 0.3, y, 0), (0, 90, 0), label='CarveRest_%d_%d' % (index, side),
                       folder='Carving')
        if carvable:
            actor = spawn(carvable, (x, y, 30 + 16), label='Carvable_%d' % length, folder='Carving')
            actor.set_editor_property('source_mesh', made['RawLog_%d' % length])
        else:
            log('CarvableLog class missing: build the C++ module first')

    # Build pad behind the start: a slightly darker flat square to lay walls on.
    spawn_mesh(cube, (-2000, -200, -49), scale=(14, 14, 1), label='BuildPad', folder='Building', material=m['pad'])

    sign('ORMAN  -  balta, 8 / 6 vuruş', (800, -1600, 450), (0, 0))
    sign('KAYALAR  -  kazma', (700, 1500, 300), (0, 0))
    sign('İSTASYONLAR  -  duvar, taş temel, kasa, çöküş', (1200, 0, 380), (0, 0), size=55)
    sign('OYMA  -  [R] araç  [Tekerlek] açı', (-900, 1100, 220), (0, 0), size=45)
    sign('İNŞA ALANI  -  [B]', (-1400, -200, 260), (0, 0), size=60)

    ok = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH)
    log('map saved', MAP_PATH, ok)


def new_template_map():
    world = unreal.EditorLoadingAndSavingUtils.new_map_from_template('/Engine/Maps/Templates/Template_Default', False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actors.get_all_level_actors():
        if isinstance(actor, (unreal.StaticMeshActor, unreal.PlayerStart)):
            actors.destroy_actor(actor)
    return world


def build_demo_map(made):
    """Small stage for the wood chain demo: three pines in front, a carving spot, AMechanicsDemo guides the player on play."""
    world = new_template_map()
    cube = unreal.load_asset('/Engine/BasicShapes/Cube')
    grid = unreal.load_asset('/Engine/EngineMaterials/WorldGridMaterial')
    spawn_mesh(cube, (0, 0, -50), scale=(70, 70, 1), label='Ground', folder='Ground', material=grid)
    spawn(unreal.PlayerStart, (0, 0, 100), label='PlayerStart')

    # Three pines to fell (6 hits each), straight ahead and to the sides; the broad tree takes 8 hits.
    spawn_mesh(made['TreePine'], (700, 0, 0), (0, 15, 0), label='Tree_Pine_Demo', folder='Trees')
    spawn_mesh(made['TreePine'], (800, -650, 0), (0, 60, 0), label='Tree_Pine_Again1', folder='Trees')
    spawn_mesh(made['TreePine'], (800, 650, 0), (0, 110, 0), label='Tree_Pine_Again2', folder='Trees')
    spawn_mesh(made['TreeBroad'], (-300, -1500, 0), (0, 40, 0), label='Tree_Broad_Manual', folder='Trees')

    demo_class = unreal.load_class(None, '/Script/MyProject2.MechanicsDemo')
    if demo_class:
        # The raw log is carved here, lying along the arrow (+X).
        spawn(demo_class, (250, 600, 0), (0, 0, 0), 'MechanicsDemo', 'Demo')
    else:
        log('MechanicsDemo class missing: build the C++ module first')

    sign('MEKANİK DEMOSU', (450, -450, 330), (0, 0), size=90)
    sign('sol üstteki görev listesini takip et  -  baştan: P sonra L', (450, -450, 250), (0, 0), size=34)
    sign('oyma noktası', (250, 600, 160), (0, 0), size=40)

    ok = unreal.EditorLoadingAndSavingUtils.save_map(world, DEMO_MAP_PATH)
    log('map saved', DEMO_MAP_PATH, ok)

def spawn_harvestable(harvest_class, mesh, harvest_type, loc, rot=(0, 0, 0), scale=(1, 1, 1),
                      label='Harvestable', folder='Resources'):
    """Replace a temporary mesh prop with the real damageable tree/rock actor."""
    source = spawn_mesh(mesh, loc, rot, scale, label='Source_' + label, folder='TempSources')
    actor = spawn(harvest_class, loc, rot, label, folder)
    if not source or not actor:
        if source:
            unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(source)
        return None
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.init_from(source.static_mesh_component, harvest_type)
    unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(source)
    return actor


def spawn_raw_carvable(carvable_class, mesh, loc, label, folder='WoodStock'):
    actor = spawn(carvable_class, loc, (0, 0, 0), label, folder)
    if actor:
        actor.set_editor_property('source_mesh', mesh)
    return actor


def spawn_path(cube, material, start, end, width=180.0, label='Path'):
    dx, dy = end[0] - start[0], end[1] - start[1]
    length = math.sqrt(dx * dx + dy * dy)
    centre = ((start[0] + end[0]) * 0.5, (start[1] + end[1]) * 0.5, -3.0)
    yaw = math.degrees(math.atan2(dy, dx))
    return spawn_mesh(cube, centre, (0, yaw, 0), (length / 100.0, width / 100.0, 0.06),
                      label=label, folder='Paths', material=material)


def load_contextual_materials():
    return {
        'ground': unreal.load_asset(MATS + '/MI_GB_Ground'),
        'trail': unreal.load_asset(MATS + '/MI_GB_Trail'),
        'pad': unreal.load_asset(MATS + '/MI_GB_BuildPad'),
        'soil': unreal.load_asset(MATS + '/MI_GB_SpikeSoil'),
    }


def build_contextual_demo_map(made, m):
    """
    A small playable scene, deliberately separate from Demo_WoodChain.
    Test fixtures in Demo_WoodChain use fixed coordinates; do not populate that map with live logs.
    """
    world = new_template_map()
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cube = unreal.load_asset('/Engine/BasicShapes/Cube')
    harvest_class = unreal.load_class(None, '/Script/MyProject2.HarvestableActor')
    carvable_class = unreal.load_class(None, '/Script/MyProject2.CarvableLog')

    # A 70 x 70 m playable valley: restrained colours and clear, walkable paths instead of a world-grid floor.
    spawn_mesh(cube, (0, 0, -50), scale=(70, 70, 1), label='Ground', folder='Ground', material=m['ground'])
    spawn(unreal.PlayerStart, (-900, 0, 100), (0, 0, 0), 'PlayerStart', 'Start')
    spawn_mesh(cube, (-900, 0, -4), scale=(5, 5, 0.08), label='StartClearing', folder='Pads', material=m['trail'])

    # Paths are only layout cues: they connect the actual interactive stations and do not block movement.
    spawn_path(cube, m['trail'], (-650, 0), (650, -650), 160, 'Path_Start_To_Forest')
    spawn_path(cube, m['trail'], (-650, 0), (-1350, 450), 160, 'Path_Start_To_Workshop')
    spawn_path(cube, m['trail'], (-650, 0), (800, 600), 160, 'Path_Start_To_BuildYard')
    spawn_path(cube, m['trail'], (800, 600), (2100, 950), 160, 'Path_BuildYard_To_Quarry')
    spawn_path(cube, m['trail'], (800, 600), (2000, -1050), 160, 'Path_BuildYard_To_StakeSoil')
    spawn_path(cube, m['trail'], (-650, 0), (-1750, -1200), 160, 'Path_Start_To_FireSite')

    # FOREST: every trunk here is the real HarvestableActor, not a decorative static mesh.
    if harvest_class:
        spawn_harvestable(harvest_class, made['TreePine'], unreal.EHarvestType.TREE,
                          (950, -1350, 0), (0, 18, 0), (1.0, 1.0, 1.0), 'Pine_Harvest_01', 'Forest')
        spawn_harvestable(harvest_class, made['TreePine'], unreal.EHarvestType.TREE,
                          (1550, -1700, 0), (0, 75, 0), (1.0, 1.0, 1.0), 'Pine_Harvest_02', 'Forest')
        spawn_harvestable(harvest_class, made['TreePine'], unreal.EHarvestType.TREE,
                          (900, -2200, 0), (0, 130, 0), (0.95, 0.95, 0.95), 'Pine_Harvest_03', 'Forest')
        spawn_harvestable(harvest_class, made['TreeBroad'], unreal.EHarvestType.TREE,
                          (1750, -2250, 0), (0, 40, 0), (1.0, 1.0, 1.0), 'Broad_Harvest_04', 'Forest')
    else:
        log('HarvestableActor class missing; forest would be scenery only')
        spawn_mesh(made['TreePine'], (950, -1350, 0), label='Pine_Fallback', folder='Forest')

    # WORKSHOP: raw full-size stock sits on two stone rests. A round log has no groove option;
    # the player must split it lengthwise to a half-log before the channel can be cut.
    spawn_mesh(cube, (-1450, 650, -5), scale=(16, 12, 0.1), label='CarvingPad', folder='Pads', material=m['pad'])
    for index, length in enumerate((500, 375, 250)):
        y = 400 + index * 260
        for side in (-1, 1):
            spawn_mesh(made['StoneBlock'], (-1450 + side * length * 0.30, y, 0), (0, 90, 0),
                       label='CarvingRest_%d_%d' % (index, side), folder='Workshop')
        if carvable_class:
            spawn_raw_carvable(carvable_class, made['RawLog_%d' % length], (-1450, y, 46),
                               'RawLog_%d' % length, 'Workshop')

    # BUILD YARD: materials are loose and selectable; there is no prebuilt hut hiding support/collapse behaviour.
    spawn_mesh(cube, (900, 600, -5), scale=(16, 12, 0.1), label='BuildYardPad', folder='Pads', material=m['pad'])
    if carvable_class:
        spawn_raw_carvable(carvable_class, made['RawLog_500'], (500, 850, 16), 'Stock_Raw_500', 'WoodStock')
        spawn_raw_carvable(carvable_class, made['RawLog_375'], (1050, 950, 16), 'Stock_Raw_375', 'WoodStock')
        spawn_raw_carvable(carvable_class, made['RawLog_250'], (1500, 850, 16), 'Stock_Raw_250', 'WoodStock')
    # One usable cart and sled sit beside the stock, not randomly scattered across the map.
    cart_class = unreal.load_class(None, '/Script/MyProject2.HaulCart')
    if cart_class:
        cart = spawn(cart_class, (650, 1450, 0), label='Working_Cart', folder='Hauling')
        sled = spawn(cart_class, (1120, 1450, 0), label='Working_Sled', folder='Hauling')
        if cart:
            cart.set_kind(unreal.EHaulKind.CART)
        if sled:
            sled.set_kind(unreal.EHaulKind.SLED)
    else:
        log('HaulCart class missing; skip cart/sled placement')

    # QUARRY: a small group of damageable rocks; break them to obtain inventory stones for the fire pit.
    spawn_mesh(cube, (2250, 1150, -5), scale=(10, 9, 0.1), label='QuarryPad', folder='Pads', material=m['trail'])
    if harvest_class:
        for index, (loc, scale) in enumerate([
            ((2100, 900, 0), (1.0, 1.0, 1.0)),
            ((2550, 1150, 0), (1.35, 1.1, 1.25)),
            ((2200, 1500, 0), (1.55, 1.4, 1.35)),
        ]):
            spawn_harvestable(harvest_class, made['Rock'], unreal.EHarvestType.ROCK, loc, (0, index * 33, 0),
                              scale, 'Rock_Harvest_%02d' % index, 'Quarry')

    # FIRE SITE: empty ground to place seven stones; don't pre-build the pit for the player.
    spawn_mesh(cube, (-1750, -1250, -5), scale=(10, 10, 0.1), label='FireSitePad', folder='Pads', material=m['soil'])
    # SPIKE PLANTING: designated soil only. No unanchored pointed logs are placed here as fake traps.
    spawn_mesh(cube, (2050, -1050, -5), scale=(12, 7, 0.1), label='StakePlantingSoil', folder='Pads', material=m['soil'])
    for index in range(3):
        spawn_mesh(cube, (1750 + index * 300, -1050, 0.5), scale=(0.35, 0.35, 0.025),
                   label='StakePlantingMark_%02d' % index, folder='PlantingMarks', material=m['trail'])

    # Only concise, contextual signs: a sign at the station entrance, not a label on every object.
    sign('DEMO / KAYNAK → İŞLE → İNŞA ET', (-850, -180, 250), (-650, 0), size=55)
    sign('ORMAN / Ağaç gövdesi → büyük kütük → taşınabilir kütük', (1120, -1000, 260), (950, -1350), size=40)
    sign('İŞLEME / Yuvarlak loga oluk yok; önce boyuna yar', (-1450, 1200, 230), (-1450, 650), size=40)
    sign('İNŞA / Direk + kiriş; desteği sökersen yükü kontrol et', (900, 1050, 250), (900, 600), size=40)
    sign('TAŞ OCAĞI / Kayayı kır; taşları topla', (2530, 1650, 230), (2250, 1150), size=40)
    sign('ATEŞ / Önce 7 taşı yerleştir, sonra yakıt ekle', (-1750, -1650, 220), (-1750, -1250), size=40)
    sign('KAZIK TOPRAĞI / Sivri uç yukarı; taban zemine bağlı olmalı', (2050, -1450, 230), (2050, -1050), size=40)
    sign('TAŞIMA / Arabaya ve kızağa yükle; kapasiteyi aşma', (900, 1700, 230), (850, 1450), size=40)

    demo_class = unreal.load_class(None, '/Script/MyProject2.MechanicsDemo')
    if demo_class:
        # The guided log lives in the start clearing, separate from the stock at the workshop.
        spawn(demo_class, (-450, 0, 0), (0, 0, 0), 'MechanicsDemo_Contextual', 'Demo')
    else:
        log('MechanicsDemo class missing: build the C++ module first')

    ok = unreal.EditorLoadingAndSavingUtils.save_map(world, CONTEXTUAL_DEMO_MAP_PATH)
    log('map saved', CONTEXTUAL_DEMO_MAP_PATH, ok)




def load_made():
    names = {
        'TreePine': 'SM_GB_Tree_Pine', 'TreeBroad': 'SM_GB_Tree_Broad', 'Rock': 'SM_GB_Rock',
        'Stone': 'SM_GB_Stone', 'StoneBlock': 'SM_GB_StoneBlock', 'Plank': 'SM_GB_Plank',
        'RawLog_500': 'SM_GB_RawLog_500', 'RawLog_375': 'SM_GB_RawLog_375', 'RawLog_250': 'SM_GB_RawLog_250',
    }
    return {key: unreal.load_asset(MESHES + '/' + name) for key, name in names.items()}


def main():
    import os
    mode = os.environ.get('GREYBOX_BUILD')
    if mode == 'demo':
        build_demo_map(load_made())
        return
    if mode == 'contextual':
        build_contextual_demo_map(load_made(), load_contextual_materials())
        return

    for path in (GB, MESHES, MATS, '/Game/Maps'):
        asset_lib.make_directory(path)

    m = {
        'bark': make_colour_material('MI_GB_Bark', (0.20, 0.12, 0.06)),
        'leaves': make_colour_material('MI_GB_Leaves', (0.10, 0.28, 0.07)),
        'pine': make_colour_material('MI_GB_Pine', (0.05, 0.18, 0.08)),
        'stone': make_colour_material('MI_GB_Stone', (0.32, 0.32, 0.30)),
        'handle': make_colour_material('MI_GB_Handle', (0.45, 0.30, 0.15)),
        'metal': make_colour_material('MI_GB_Metal', (0.12, 0.12, 0.13)),
        'pad': make_colour_material('MI_GB_BuildPad', (0.18, 0.16, 0.13)),
        'ground': make_colour_material('MI_GB_Ground', (0.16, 0.19, 0.14)),
        'trail': make_colour_material('MI_GB_Trail', (0.28, 0.23, 0.17)),
        'soil': make_colour_material('MI_GB_SpikeSoil', (0.13, 0.10, 0.07)),
        'cut': unreal.load_asset('/Game/Building/Materials/M_CutWood'),
    }
    if m['cut'] is None:
        log('M_CutWood missing, using handle colour for cut faces')
        m['cut'] = m['handle']

    made = build_meshes(m)
    build_map(made, m)
    build_demo_map(made)
    build_contextual_demo_map(made, m)


try:
    main()
except Exception:
    import traceback
    log(traceback.format_exc())
    raise
finally:
    import os
    with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'build_greybox.log'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(LOG))
