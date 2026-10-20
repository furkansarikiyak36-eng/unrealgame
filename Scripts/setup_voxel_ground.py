"""Editor: voxel ground materials (world-aligned, no UVs needed) and PerfTest set up for the diggable voxel ground.
- /Game/Excavation/M_VoxelGround (triplanar base colour + roughness), MI_VoxelSoil (Megascans Soil_Mud), MI_VoxelRock (Rough_Rock)
- PerfTest: the old ground cube (GrassGround) goes 3.1 m down, under the deepest bedrock, so it never shows in a hole;
  the voxel surface streams round the player (7x7 chunks of 16 m, surface layer and the one above).
py ".../setup_voxel_ground.py" -> Saved/setup_voxel_ground.json. Map changes only on /Game/Maps/PerfTest."""
import json, os
import unreal
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'setup_voxel_ground.json')
res = {}
at = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

def get_or_create(name, path, cls, factory):
    full = '%s/%s' % (path, name)
    if eal.does_asset_exist(full):
        return unreal.load_asset(full)
    return at.create_asset(name, path, cls, factory)

m = get_or_create('M_VoxelGround', '/Game/Excavation', unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
# Triplanar from the surface's own geometry (screen derivatives), so it does not depend on mesh normals or UVs.
tex = mel.create_material_expression(m, unreal.MaterialExpressionTextureObjectParameter, -900, -100)
tex.set_editor_property('parameter_name', 'BaseColorTex')
tex.set_editor_property('texture', unreal.load_asset('/Game/Fab/Megascans/Surfaces/Soil_Mud_pjuph20/Low/pjuph20_tier_3/Textures/T_pjuph20_1K_B'))
size = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, 120)
size.set_editor_property('parameter_name', 'TileSizeCm'); size.set_editor_property('default_value', 200.0)
wp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -1300, 250)
cu = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -1000, 400)
i = unreal.CustomInput(); i.set_editor_property('input_name', 'WorldPos')
cu.set_editor_property('inputs', [i])
cu.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
cu.set_editor_property('code', '''float3 N = normalize(cross(ddx(WorldPos), ddy(WorldPos)));
float3 W = pow(abs(N), 4.0);
return W / max(W.x + W.y + W.z, 1e-4);''')
q = mel.create_material_expression(m, unreal.MaterialExpressionDivide, -1100, 100)
mel.connect_material_expressions(wp, '', q, 'A'); mel.connect_material_expressions(size, '', q, 'B')
mel.connect_material_expressions(wp, '', cu, 'WorldPos')
samples = []
for k, (r, g, b) in enumerate(((False, True, True), (True, False, True), (True, True, False))):
    mk = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, -900, -300 + 160 * k)
    mk.set_editor_property('r', r); mk.set_editor_property('g', g); mk.set_editor_property('b', b)
    mel.connect_material_expressions(q, '', mk, '')
    ts = mel.create_material_expression(m, unreal.MaterialExpressionTextureSample, -700, -300 + 160 * k)
    mel.connect_material_expressions(mk, '', ts, 'UVs')
    mel.connect_material_expressions(tex, '', ts, 'Tex')
    wm = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, -700, 450 + 60 * k)
    wm.set_editor_property('r', k == 0); wm.set_editor_property('g', k == 1); wm.set_editor_property('b', k == 2)
    mel.connect_material_expressions(cu, '', wm, '')
    ml = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -500, -300 + 160 * k)
    mel.connect_material_expressions(ts, 'RGB', ml, 'A'); mel.connect_material_expressions(wm, '', ml, 'B')
    samples.append(ml)
a1 = mel.create_material_expression(m, unreal.MaterialExpressionAdd, -400, -200)
mel.connect_material_expressions(samples[0], '', a1, 'A'); mel.connect_material_expressions(samples[1], '', a1, 'B')
a2 = mel.create_material_expression(m, unreal.MaterialExpressionAdd, -350, -100)
mel.connect_material_expressions(a1, '', a2, 'A'); mel.connect_material_expressions(samples[2], '', a2, 'B')
tint = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -500, 250)
tint.set_editor_property('parameter_name', 'Tint'); tint.set_editor_property('default_value', unreal.LinearColor(1, 1, 1, 1))
mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -250, 0)
amb = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -250, 200)
amb.set_editor_property('parameter_name', 'AmbientFill'); amb.set_editor_property('default_value', 0.06)
emi = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -100, 200)
rough = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -250, 350)
rough.set_editor_property('parameter_name', 'Roughness'); rough.set_editor_property('default_value', 0.92)
res['c4'] = mel.connect_material_expressions(a2, '', mul, 'A')
res['c5'] = mel.connect_material_expressions(tint, '', mul, 'B')
res['c6'] = mel.connect_material_property(mul, '', unreal.MaterialProperty.MP_BASE_COLOR)
res['c7'] = mel.connect_material_expressions(mul, '', emi, 'A')
res['c8'] = mel.connect_material_expressions(amb, '', emi, 'B')
res['c9'] = mel.connect_material_property(emi, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
res['c10'] = mel.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(m)
eal.save_loaded_asset(m)

def mi(name, texpath, tile, tint):
    i = get_or_create(name, '/Game/Excavation', unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(i, m)
    mel.set_material_instance_texture_parameter_value(i, 'BaseColorTex', unreal.load_asset(texpath))
    mel.set_material_instance_scalar_parameter_value(i, 'TileSizeCm', tile)
    mel.set_material_instance_vector_parameter_value(i, 'Tint', tint)
    eal.save_loaded_asset(i)
    return i.get_path_name()
res['soil'] = mi('MI_VoxelSoil', '/Game/Fab/Megascans/Surfaces/Soil_Mud_pjuph20/Low/pjuph20_tier_3/Textures/T_pjuph20_1K_B', 200.0, unreal.LinearColor(0.85, 0.8, 0.75, 1))
res['rock'] = mi('MI_VoxelRock', '/Game/Fab/Megascans/Surfaces/Rough_Rock_xbkgbab/Low/xbkgbab_tier_3/Textures/T_xbkgbab_1K_B', 250.0, unreal.LinearColor(1, 1, 1, 1))

cdo = unreal.get_default_object(unreal.ExcavationTerrainChunkSubsystem)
cdo.set_editor_property('horizontal_chunk_radius', 3)
cdo.set_editor_property('underground_chunk_layers', 1)
cdo.set_editor_property('max_chunks_spawned_per_tick', 4)

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
path = world.get_outer().get_path_name()
res['map'] = path
if path == '/Game/Maps/PerfTest':
    for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if a.get_actor_label() == 'GrassGround':
            loc = a.get_actor_location()
            if loc.z > -100:
                a.set_actor_location(unreal.Vector(loc.x, loc.y, loc.z - 310.0), False, False)
            res['ground_z'] = a.get_actor_location().z
    res['saved'] = unreal.EditorLoadingAndSavingUtils.save_current_level()
json.dump(res, open(OUT, 'w'), indent=1, default=str)
