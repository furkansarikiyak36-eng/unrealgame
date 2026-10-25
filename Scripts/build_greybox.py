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
Set GREYBOX_BUILD=raw (or run Scripts/build_raw_demo.py) to rebuild only the Demo_RawMaterials map.
Set GREYBOX_BUILD=main (or run Scripts/build_main_map.py) to rebuild only the Main sandbox map.
"""
import math
import random

import unreal

GB = '/Game/Greybox'
MESHES = GB + '/Meshes'
MATS = GB + '/Materials'
MAP_PATH = '/Game/Maps/Greybox_Mechanics'
DEMO_MAP_PATH = '/Game/Maps/Demo_WoodChain'
RAW_DEMO_MAP_PATH = '/Game/Maps/Demo_RawMaterials'
MAIN_MAP_PATH = '/Game/Maps/Main'

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


def build_log_hollow(materials):
    """Build the greybox hollow log as an open channel with a rounded inner bed."""
    bark, cut = materials
    log_length, log_diameter = 390.0, 70.0
    mesh = new_mesh()
    append_log(mesh, log_length, log_diameter)

    # Leave a visible wood base while making the channel substantially deeper.
    channel_length = log_length - 40.0
    channel_radius = log_diameter * 0.31
    channel_center_z = -log_diameter * 0.09

    # The cylinder removes the rounded lower half; the box opens the top and
    # leaves straight sides, producing a U-shaped trough rather than a flat cut.
    round_tool = new_mesh()
    prim.append_cylinder(round_tool, opts(1), xf((0, 0, channel_center_z), ALONG_X),
                         channel_radius, channel_length, 32, 0, True,
                         unreal.GeometryScriptPrimitiveOriginMode.CENTER)
    subtract(mesh, round_tool)

    opening_tool = new_mesh()
    prim.append_box(opening_tool, opts(1), xf((0, 0, channel_center_z)),
                    channel_length, channel_radius * 2.0, log_diameter * 2.0,
                    0, 0, 0, unreal.GeometryScriptPrimitiveOriginMode.BASE)
    subtract(mesh, opening_tool)
    return save_mesh('SM_GB_LogHollow', mesh, [bark, cut], hulls=8)


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

    made['LogHollow'] = build_log_hollow([bark, cut])

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


# The default text render font has no glyphs for these Turkish letters; they show as gaps.
SIGN_FOLD = str.maketrans({'İ': 'I', 'ı': 'i', 'Ş': 'S', 'ş': 's', 'Ğ': 'G', 'ğ': 'g'})


def sign(text, loc, face_towards=(0, 0), size=70.0, colour=(255, 240, 200)):
    text = text.translate(SIGN_FOLD)
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


# Raw material sources around the start, each with its name and what it gives. Heights put the engine shape's
# centred pivot on the ground (see ARawMaterialNode's looks).
RAW_DEMO_SOURCES = [
    ('FIBER_BUSH', 'LİF ÇALISI', 'bitki lifi, bazen asma', (700, -900), 40),
    ('VINES', 'ASMA', 'asma (eklem bağı)', (900, -450), 80),
    ('DEAD_TREE', 'KURU AĞAÇ', 'ağaç kabuğu, bazen reçine', (1000, 0), 130),
    ('CLAY_BANK', 'KİL YATAĞI', 'kil (kütük arası sıva)', (900, 450), 12.5),
    ('FLINT_OUTCROP', 'ÇAKMAK TAŞI', 'çakmak taşı', (700, 900), 35),
    ('BONE_PILE', 'KEMİK YIĞINI', 'kemik', (300, 1100), 12.5),
]


def rerun(actor):
    """Re-applies an actor's construction after its properties were set (look and default drops by kind)."""
    if hasattr(actor, 'rerun_construction_scripts'):
        actor.rerun_construction_scripts()


def set_raw_props(node, kind, **props):
    """Sets the source kind and other properties; empty drops then take the kind's defaults on construction.
    A class added by Live Coding has no Python bindings until the editor restarts: the properties are then left to
    the editor's property tools (the actor label carries the kind) and False is returned."""
    enum = getattr(unreal, 'RawSourceKind', None)
    if enum is None:
        log('RawSourceKind not in Python yet:', node.get_actor_label(), 'needs Kind', kind, props)
        return False
    node.set_editor_property('kind', getattr(enum, kind))
    node.set_editor_property('drops', [])
    for name, value in props.items():
        node.set_editor_property(name, value)
    rerun(node)
    return True


def build_raw_demo_map():
    """Raw material stage: six sources in an arc in front of the start, [E] gathers, [I] twists fibre into rope."""
    node_class = unreal.load_class(None, '/Script/MyProject2.RawMaterialNode')
    if node_class is None:
        raise RuntimeError('RawMaterialNode class missing: build the C++ module (or Live Coding) first.')
    world = new_template_map()
    cube = unreal.load_asset('/Engine/BasicShapes/Cube')
    grid = unreal.load_asset('/Engine/EngineMaterials/WorldGridMaterial')
    spawn_mesh(cube, (0, 0, -50), scale=(40, 40, 1), label='Ground', folder='Ground', material=grid)
    spawn(unreal.PlayerStart, (0, 0, 100), label='PlayerStart')

    for kind, name, gives, (x, y), z in RAW_DEMO_SOURCES:
        node = spawn(node_class, (x, y, z), (0, 0, 0), 'Raw_' + kind, 'RawMaterials')
        # Quick regrowth so emptying and refilling a source can be watched in the demo.
        set_raw_props(node, kind, respawn_seconds=20.0)
        sign(name, (x, y, z * 2 + 120), (0, 0), size=45)
        sign(gives, (x, y, z * 2 + 85), (0, 0), size=26, colour=(200, 230, 255))

    # One source that never runs out, to fill the pack for rope.
    endless = spawn(node_class, (-300, -900, 40), (0, 0, 0), 'Raw_FIBER_BUSH_Endless', 'RawMaterials')
    set_raw_props(endless, 'FIBER_BUSH', max_supply=0)
    sign('SONSUZ LİF', (-300, -900, 200), (0, 0), size=40)

    # High above the arc, clear of the sources' own signs.
    sign('HAM MADDE DEMOSU', (1700, 0, 760), (0, 0), size=90)
    sign('[E] topla  -  [Tab] çanta  -  [I] ip: 3 lif ya da 2 asma', (1700, 0, 670), (0, 0), size=36)
    sign('kaynak tükenince küçülür, 20 sn sonra yenilenir', (1700, 0, 615), (0, 0), size=30, colour=(200, 230, 255))

    ok = unreal.EditorLoadingAndSavingUtils.save_map(world, RAW_DEMO_MAP_PATH)
    log('map saved', RAW_DEMO_MAP_PATH, ok)


# Sandbox platforms: top height (cm) and centre; 4 x 4 m, solid down to the ground.
MAIN_PLATFORMS = [(25, (1000, -1000)), (50, (1000, 0)), (100, (1000, 1000)), (200, (-1000, 1000))]


def build_main_map():
    """Empty sandbox to place things on: a large grid floor, platforms at a few heights, a ramp and a wide deck."""
    world = new_template_map()
    cube = unreal.load_asset('/Engine/BasicShapes/Cube')
    grid = unreal.load_asset('/Engine/EngineMaterials/WorldGridMaterial')
    plain = unreal.load_asset('/Engine/BasicShapes/BasicShapeMaterial')
    # 80 x 80 m, top at z = 0; the world grid material draws 1 m lines for placing.
    spawn_mesh(cube, (0, 0, -50), scale=(80, 80, 1), label='Ground', folder='Ground', material=grid)
    spawn(unreal.PlayerStart, (0, 0, 100), label='PlayerStart')

    for height, (x, y) in MAIN_PLATFORMS:
        spawn_mesh(cube, (x, y, height / 2.0), scale=(4, 4, height / 100.0),
                   label='Platform_%dcm' % height, folder='Platforms', material=grid)
        sign('%d cm' % height if height < 100 else '%d m' % (height // 100), (x, y, height + 60), (0, 0), size=50)

    # A ramp up to the 1 m platform from the start side: 6 m across, about 9.5 degrees.
    run, rise = 600.0, 100.0
    slope = math.degrees(math.atan2(rise, run))
    spawn_mesh(cube, (1000 - 200 - run / 2.0, 1000, rise / 2.0 - 5), (slope, 0, 0),
               scale=(math.hypot(run, rise) / 100.0, 2, 0.1), label='Ramp_1m', folder='Platforms', material=grid)

    # A wide, low deck to build on: 10 x 10 m, 10 cm high.
    spawn_mesh(cube, (-1300, -1300, 5), scale=(10, 10, 0.1), label='Deck_10m', folder='Platforms', material=plain)
    sign('10 x 10 m döşeme', (-1300, -1300, 80), (0, 0), size=50)

    ok = unreal.EditorLoadingAndSavingUtils.save_map(world, MAIN_MAP_PATH)
    log('map saved', MAIN_MAP_PATH, ok)


def load_made():
    names = {'TreePine': 'SM_GB_Tree_Pine', 'TreeBroad': 'SM_GB_Tree_Broad'}
    return {key: unreal.load_asset(MESHES + '/' + name) for key, name in names.items()}


def main():
    import os
    build_mode = os.environ.get('GREYBOX_BUILD')
    if build_mode == 'demo':
        build_demo_map(load_made())
        return
    if build_mode == 'main':
        build_main_map()
        return
    if build_mode == 'raw':
        build_raw_demo_map()
        return
    if build_mode == 'hollow':
        bark = unreal.load_asset(MATS + '/MI_GB_Bark')
        cut = unreal.load_asset('/Game/Building/Materials/M_CutWood')
        if bark is None or cut is None:
            raise RuntimeError('Required bark or cut-wood material is missing.')
        build_log_hollow([bark, cut])
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
        'cut': unreal.load_asset('/Game/Building/Materials/M_CutWood'),
    }
    if m['cut'] is None:
        log('M_CutWood missing, using handle colour for cut faces')
        m['cut'] = m['handle']

    made = build_meshes(m)
    build_map(made, m)
    build_demo_map(made)


if __name__ == '__main__':
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
