"""Textures the hand-made UnrealExport logs: one master material, five instances, assigned by material slot.

Run in the open editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/create_rawlog_materials.py"

The logs' UVs are real-size (measured in Blender: 1 UV unit = 110 cm on every slot; U along the log, the bark's V
wraps the circumference once, the end faces are centred on the log axis). M_RawLog uses that:
- side faces: a texture swapped to run along the log (bark furrows and wood fibre follow the length), tiled with a
  whole number of repeats around the bark so there is no seam;
- end faces (local normal along X, the log's axis): growth rings drawn from the UV centre outwards (colours are
  linear: sRGB-looking values come out near white), so a sawn end
  shows rings whatever slot it carries (Plank and Kalas use one slot for every face).
Instances: Bark (Fab Spruce Bark, warmed), Cut (fresh sawn wood), Hewn (adzed, a little darker), Peeled (debarked,
lighter and smoother), PeeledCut (rings of peeled logs). Assets in /Game/UnrealExport/Materials; the meshes are
saved with the new slots (a re-import of the FBX files resets them). Report: Saved/create_rawlog_materials.json.
"""
import json
import os

import unreal

DEST = '/Game/UnrealExport/Materials'
FENCE = '/Game/ProceduralNtr_vol2/pnp_v2_assets/Assets/wood_fence/Textures/'
SPRUCE = '/Game/Fab/Megascans/Surfaces/Spruce_Bark_ugqkajmew/Low/ugqkajmew_tier_3/Textures/T_ugqkajmew_1K_'
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'create_rawlog_materials.json')

lib = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def tex(path):
    return unreal.load_asset(path)


def node(mat, cls, x, y, **props):
    e = lib.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def link(a, b, pin, out=''):
    lib.connect_material_expressions(a, out, b, pin)


def scalar(mat, name, value, x, y):
    return node(mat, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value)


def vector(mat, name, rgb, x, y):
    return node(mat, unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name,
                default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))


def binop(mat, cls, a, b, x, y):
    e = node(mat, cls, x, y)
    link(a, e, 'A')
    link(b, e, 'B')
    return e


def lerp(mat, a, b, alpha, x, y):
    e = node(mat, unreal.MaterialExpressionLinearInterpolate, x, y)
    link(a, e, 'A')
    link(b, e, 'B')
    link(alpha, e, 'Alpha')
    return e


def const(mat, value, x, y):
    return node(mat, unreal.MaterialExpressionConstant, x, y, r=value)


def unary(mat, cls, a, x, y, **props):
    e = node(mat, cls, x, y, **props)
    link(a, e, '')
    return e


def end_grain(mat, X, Y):
    """Colour of a sawn end, in polar coordinates around the log axis (the mesh's local X axis; every piece has its
    pivot on it, the end-face UVs are not centred after import). Rings: a wide light earlywood band ending in a thin
    dark latewood band, the radius bent by a few smooth angular waves (no noise speckle); rays: the fence texture's
    fibre sampled along the radius; darker heartwood towards the pith and a thin dark cambium rim at the bark."""
    lpos = node(mat, unreal.MaterialExpressionLocalPosition, X, Y)
    ly = node(mat, unreal.MaterialExpressionComponentMask, X + 200, Y - 40, r=False, g=True, b=False, a=False)
    lz = node(mat, unreal.MaterialExpressionComponentMask, X + 200, Y + 40, r=False, g=False, b=True, a=False)
    link(lpos, ly, '')
    link(lpos, lz, '')
    lyz = binop(mat, unreal.MaterialExpressionAppendVector, ly, lz, X + 400, Y)
    r = unary(mat, unreal.MaterialExpressionSquareRoot,
              binop(mat, unreal.MaterialExpressionDotProduct, lyz, lyz, X + 600, Y - 80), X + 800, Y - 80)
    theta = node(mat, unreal.MaterialExpressionArctangent2, X + 600, Y + 80)
    link(lz, theta, 'Y')
    link(ly, theta, 'X')

    # Smooth angular waves bending the rings (cm), growing outwards.
    bend = None
    for i, (k, phase, amp) in enumerate(((3.0, 1.3, 'Ring Wave 3'), (5.0, 0.4, 'Ring Wave 5'), (11.0, 2.1, 'Ring Wave 11'))):
        arg = binop(mat, unreal.MaterialExpressionAdd,
                    binop(mat, unreal.MaterialExpressionMultiply, theta, const(mat, k, X + 800, Y + 200 + i * 120),
                          X + 1000, Y + 200 + i * 120),
                    const(mat, phase, X + 1000, Y + 260 + i * 120), X + 1200, Y + 200 + i * 120)
        wave = unary(mat, unreal.MaterialExpressionSine, arg, X + 1400, Y + 200 + i * 120, period=6.2831853)
        term = binop(mat, unreal.MaterialExpressionMultiply, wave,
                     scalar(mat, amp, (0.45, 0.25, 0.1)[i], X + 1400, Y + 260 + i * 120), X + 1600, Y + 200 + i * 120)
        bend = term if bend is None else binop(mat, unreal.MaterialExpressionAdd, bend, term, X + 1800, Y + 200 + i * 120)
    radius = scalar(mat, 'Log Radius Cm', 17.5, X + 800, Y - 200)
    rel = unary(mat, unreal.MaterialExpressionSaturate,
                binop(mat, unreal.MaterialExpressionDivide, r, radius, X + 1000, Y - 160), X + 1200, Y - 160)
    r_bent = binop(mat, unreal.MaterialExpressionAdd, r,
                   binop(mat, unreal.MaterialExpressionMultiply, bend, rel, X + 2000, Y + 200), X + 2200, Y - 40)

    # Rings: earlywood light, latewood a thin dark band at the end of each ring.
    spacing = scalar(mat, 'Ring Spacing Cm', 0.7, X + 2200, Y + 80)
    t = unary(mat, unreal.MaterialExpressionFrac,
              binop(mat, unreal.MaterialExpressionDivide, r_bent, spacing, X + 2400, Y), X + 2600, Y)
    late_start = scalar(mat, 'Latewood Start', 0.72, X + 2600, Y + 120)
    late = unary(mat, unreal.MaterialExpressionSaturate,
                 binop(mat, unreal.MaterialExpressionDivide,
                       binop(mat, unreal.MaterialExpressionSubtract, t, late_start, X + 2800, Y),
                       const(mat, 0.2, X + 2800, Y + 120), X + 3000, Y), X + 3200, Y)
    early_c = vector(mat, 'Ring Light', (0.36, 0.21, 0.09), X + 3200, Y - 200)
    late_c = vector(mat, 'Ring Dark', (0.16, 0.08, 0.03), X + 3200, Y + 160)
    col = lerp(mat, early_c, late_c, late, X + 3400, Y)

    # Rays: the fence texture's vertical fibre laid along the radius (angle -> U, radius -> V).
    ray_u = binop(mat, unreal.MaterialExpressionMultiply, theta, scalar(mat, 'Ray Count', 1.2, X + 2200, Y + 600),
                  X + 2400, Y + 560)
    ray_v = binop(mat, unreal.MaterialExpressionMultiply, r, const(mat, 0.02, X + 2200, Y + 700), X + 2400, Y + 660)
    ray_uv = binop(mat, unreal.MaterialExpressionAppendVector, ray_u, ray_v, X + 2600, Y + 600)
    rays = node(mat, unreal.MaterialExpressionTextureSample, X + 2800, Y + 560, texture=tex(FENCE + 'wooden_fence_albedo'),
                sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    link(ray_uv, rays, 'UVs')
    ray_r = node(mat, unreal.MaterialExpressionComponentMask, X + 3000, Y + 560, r=True, g=False, b=False, a=False)
    link(rays, ray_r, '')
    ray_k = lerp(mat, const(mat, 0.75, X + 3000, Y + 660), const(mat, 1.35, X + 3000, Y + 720),
                 binop(mat, unreal.MaterialExpressionMultiply, ray_r, const(mat, 2.0, X + 3000, Y + 780), X + 3200, Y + 760),
                 X + 3400, Y + 600)
    col = binop(mat, unreal.MaterialExpressionMultiply, col, ray_k, X + 3600, Y + 100)

    # Heartwood darker towards the pith, a thin dark cambium rim at the bark.
    heart = binop(mat, unreal.MaterialExpressionMultiply,
                  unary(mat, unreal.MaterialExpressionSaturate,
                        binop(mat, unreal.MaterialExpressionSubtract, const(mat, 1.0, X + 3400, Y + 900),
                              binop(mat, unreal.MaterialExpressionDivide, rel, const(mat, 0.55, X + 3400, Y + 1000),
                                    X + 3600, Y + 960), X + 3800, Y + 920), X + 4000, Y + 920),
                  scalar(mat, 'Heartwood Darken', 0.35, X + 4000, Y + 1020), X + 4200, Y + 940)
    col = binop(mat, unreal.MaterialExpressionMultiply, col,
                binop(mat, unreal.MaterialExpressionSubtract, const(mat, 1.0, X + 4200, Y + 860), heart, X + 4400, Y + 900),
                X + 4600, Y + 100)
    rim = unary(mat, unreal.MaterialExpressionSaturate,
                binop(mat, unreal.MaterialExpressionDivide,
                      binop(mat, unreal.MaterialExpressionSubtract, r,
                            binop(mat, unreal.MaterialExpressionSubtract, radius, const(mat, 1.0, X + 4400, Y - 300),
                                  X + 4600, Y - 300), X + 4800, Y - 260),
                      const(mat, 1.0, X + 4800, Y - 160), X + 5000, Y - 240), X + 5200, Y - 240)
    rim_c = vector(mat, 'Cambium Color', (0.07, 0.04, 0.02), X + 5200, Y - 120)
    return lerp(mat, col, rim_c, rim, X + 5400, Y)


def end_slice(mat, X, Y):
    """The photographed log slice (T_EndGrain_Slice_C/N) laid on a sawn end around the log axis: local Y/Z in cm
    times 'End Texture Scale' (UV per cm; 0.0263 puts a 17.5 cm radius at 0.46, just inside the slice's narrower
    vertical radius) around the texture centre, turned by an angle taken from the actor's position so neighbouring
    logs do not show the same slice the same way up. Returns colour (times 'End Tint') and a softened normal."""
    lpos = node(mat, unreal.MaterialExpressionLocalPosition, X, Y)
    ly = node(mat, unreal.MaterialExpressionComponentMask, X + 200, Y - 40, r=False, g=True, b=False, a=False)
    lz = node(mat, unreal.MaterialExpressionComponentMask, X + 200, Y + 40, r=False, g=False, b=True, a=False)
    link(lpos, ly, '')
    link(lpos, lz, '')
    # Per-actor turn: frac(dot(actor position, k)) * 2 pi.
    opos = node(mat, unreal.MaterialExpressionObjectPositionWS, X, Y + 200)
    seed = binop(mat, unreal.MaterialExpressionDotProduct, opos,
                 node(mat, unreal.MaterialExpressionConstant3Vector, X, Y + 300,
                      constant=unreal.LinearColor(0.0137, 0.0071, 0.0029, 1)), X + 200, Y + 220)
    angle = binop(mat, unreal.MaterialExpressionMultiply, unary(mat, unreal.MaterialExpressionFrac, seed, X + 400, Y + 220),
                  const(mat, 6.2831853, X + 400, Y + 320), X + 600, Y + 240)
    sin_a = unary(mat, unreal.MaterialExpressionSine, angle, X + 800, Y + 200, period=6.2831853)
    cos_a = unary(mat, unreal.MaterialExpressionCosine, angle, X + 800, Y + 300, period=6.2831853)
    ry = binop(mat, unreal.MaterialExpressionSubtract, binop(mat, unreal.MaterialExpressionMultiply, ly, cos_a, X + 1000, Y),
               binop(mat, unreal.MaterialExpressionMultiply, lz, sin_a, X + 1000, Y + 80), X + 1200, Y + 40)
    rz = binop(mat, unreal.MaterialExpressionAdd, binop(mat, unreal.MaterialExpressionMultiply, ly, sin_a, X + 1000, Y + 160),
               binop(mat, unreal.MaterialExpressionMultiply, lz, cos_a, X + 1000, Y + 240), X + 1200, Y + 200)
    scale = scalar(mat, 'End Texture Scale', 0.0263, X + 1200, Y + 320)
    half = const(mat, 0.5, X + 1200, Y + 400)
    u = binop(mat, unreal.MaterialExpressionAdd, binop(mat, unreal.MaterialExpressionMultiply, ry, scale, X + 1400, Y + 40),
              half, X + 1600, Y + 40)
    v = binop(mat, unreal.MaterialExpressionSubtract, half,
              binop(mat, unreal.MaterialExpressionMultiply, rz, scale, X + 1400, Y + 200), X + 1600, Y + 200)
    uv = binop(mat, unreal.MaterialExpressionAppendVector, u, v, X + 1800, Y + 120)
    tcol = node(mat, unreal.MaterialExpressionTextureSampleParameter2D, X + 2000, Y, parameter_name='End Grain Color',
                texture=tex('/Game/UnrealExport/Textures/T_EndGrain_Slice_C'),
                sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    tnrm = node(mat, unreal.MaterialExpressionTextureSampleParameter2D, X + 2000, Y + 300, parameter_name='End Grain Normal',
                texture=tex('/Game/UnrealExport/Textures/T_EndGrain_Slice_N'),
                sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    link(uv, tcol, 'UVs')
    link(uv, tnrm, 'UVs')
    col = binop(mat, unreal.MaterialExpressionMultiply, tcol, vector(mat, 'End Tint', (1.0, 1.0, 1.0), X + 2000, Y - 160),
                X + 2300, Y)
    flat = node(mat, unreal.MaterialExpressionConstant3Vector, X + 2000, Y + 560, constant=unreal.LinearColor(0, 0, 1, 1))
    nrm = lerp(mat, flat, tnrm, scalar(mat, 'End Normal Strength', 0.5, X + 2000, Y + 660), X + 2300, Y + 400)
    return col, nrm


def build_master(path):
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    name = path.rsplit('/', 1)[-1]
    mat = tools.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    X = -1600
    # UV, optionally swapped (U <-> V) so a texture's vertical grain runs along the log, then tiled.
    uv = node(mat, unreal.MaterialExpressionTextureCoordinate, X, 0)
    u = node(mat, unreal.MaterialExpressionComponentMask, X + 200, -60, r=True, g=False, b=False, a=False)
    v = node(mat, unreal.MaterialExpressionComponentMask, X + 200, 60, r=False, g=True, b=False, a=False)
    link(uv, u, '')
    link(uv, v, '')
    vu = binop(mat, unreal.MaterialExpressionAppendVector, v, u, X + 400, 0)
    swap = scalar(mat, 'Swap UV', 1.0, X + 400, 140)
    uvs = lerp(mat, uv, vu, swap, X + 600, 0)
    tile_u = scalar(mat, 'Tile U', 1.0, X + 600, 160)
    tile_v = scalar(mat, 'Tile V', 1.0, X + 600, 260)
    tiles = binop(mat, unreal.MaterialExpressionAppendVector, tile_u, tile_v, X + 800, 200)
    uvt = binop(mat, unreal.MaterialExpressionMultiply, uvs, tiles, X + 1000, 0)

    base = node(mat, unreal.MaterialExpressionTextureSampleParameter2D, X + 1200, -400, parameter_name='Base Color',
                texture=tex(FENCE + 'wooden_fence_albedo'), sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    nrm = node(mat, unreal.MaterialExpressionTextureSampleParameter2D, X + 1200, -100, parameter_name='Normal',
               texture=tex(FENCE + 'wooden_fence_normal'), sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    orm = node(mat, unreal.MaterialExpressionTextureSampleParameter2D, X + 1200, 200, parameter_name='ORM',
               texture=tex(SPRUCE + 'ORM'), sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    for t in (base, nrm, orm):
        link(uvt, t, 'UVs')
    tint = vector(mat, 'Tint', (1.0, 1.0, 1.0), X + 1200, -560)
    side_col = binop(mat, unreal.MaterialExpressionMultiply, base, tint, X + 1500, -450)

    # Roughness / AO: from the ORM texture, or a constant when the texture set has none.
    use_orm = scalar(mat, 'Use ORM', 0.0, X + 1400, 420)
    rough_c = scalar(mat, 'Roughness', 0.8, X + 1400, 520)
    orm_r = node(mat, unreal.MaterialExpressionComponentMask, X + 1450, 260, r=True, g=False, b=False, a=False)
    orm_g = node(mat, unreal.MaterialExpressionComponentMask, X + 1450, 340, r=False, g=True, b=False, a=False)
    link(orm, orm_r, '')
    link(orm, orm_g, '')
    side_rough = lerp(mat, rough_c, orm_g, use_orm, X + 1700, 400)
    one = node(mat, unreal.MaterialExpressionConstant, X + 1450, 600, r=1.0)
    ao = lerp(mat, one, orm_r, use_orm, X + 1700, 560)

    ring_col = end_grain(mat, X, 800)

    # End-face mask: the vertex normal in the mesh's own space points along X (the log's axis).
    nws = node(mat, unreal.MaterialExpressionVertexNormalWS, X, 1200)
    nloc = node(mat, unreal.MaterialExpressionTransform, X + 200, 1200,
                transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD,
                transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_LOCAL)
    link(nws, nloc, '')
    nx = node(mat, unreal.MaterialExpressionComponentMask, X + 400, 1200, r=True, g=False, b=False, a=False)
    link(nloc, nx, '')
    nabs = node(mat, unreal.MaterialExpressionAbs, X + 600, 1200)
    link(nx, nabs, '')
    thr = node(mat, unreal.MaterialExpressionConstant, X + 600, 1300, r=0.7)
    over = binop(mat, unreal.MaterialExpressionSubtract, nabs, thr, X + 800, 1220)
    ten = node(mat, unreal.MaterialExpressionConstant, X + 800, 1320, r=10.0)
    scaled = binop(mat, unreal.MaterialExpressionMultiply, over, ten, X + 1000, 1240)
    sat = node(mat, unreal.MaterialExpressionSaturate, X + 1200, 1240)
    link(scaled, sat, '')
    ring_on = scalar(mat, 'Rings On End Faces', 1.0, X + 1200, 1340)
    end_mask = binop(mat, unreal.MaterialExpressionMultiply, sat, ring_on, X + 1400, 1260)

    flat = node(mat, unreal.MaterialExpressionConstant3Vector, X + 1700, -60, constant=unreal.LinearColor(0, 0, 1, 1))
    end_rough = scalar(mat, 'End Roughness', 0.85, X + 1900, 520)
    slice_col, slice_nrm = end_slice(mat, X, 1800)
    use_slice = scalar(mat, 'Use End Texture', 1.0, X + 1900, 1700)
    end_col = lerp(mat, ring_col, slice_col, use_slice, X + 2000, 1500)
    end_nrm = lerp(mat, flat, slice_nrm, use_slice, X + 2000, 1700)
    out_col = lerp(mat, side_col, end_col, end_mask, X + 2100, -300)
    out_nrm = lerp(mat, nrm, end_nrm, end_mask, X + 2100, -80)
    out_rough = lerp(mat, side_rough, end_rough, end_mask, X + 2100, 420)
    lib.connect_material_property(out_col, '', unreal.MaterialProperty.MP_BASE_COLOR)
    lib.connect_material_property(out_nrm, '', unreal.MaterialProperty.MP_NORMAL)
    lib.connect_material_property(out_rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    lib.connect_material_property(ao, '', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, False)
    return mat


def instance(master, name, scalars, vectors, textures):
    path = DEST + '/' + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    mi = tools.create_asset(name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    lib.set_material_instance_parent(mi, master)
    for k, v in scalars.items():
        lib.set_material_instance_scalar_parameter_value(mi, k, v)
    for k, v in vectors.items():
        lib.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    for k, v in textures.items():
        lib.set_material_instance_texture_parameter_value(mi, k, tex(v))
    lib.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_loaded_asset(mi, False)
    return mi


def main():
    master = build_master(DEST + '/M_RawLog')
    wood_tex = {'Base Color': FENCE + 'wooden_fence_albedo', 'Normal': FENCE + 'wooden_fence_normal'}
    mis = {
        # Spruce bark: furrows along the log, one repeat round the 110 cm circumference, ~1.8 m along it.
        'Bark': instance(master, 'MI_RawLog_Bark', {'Swap UV': 1.0, 'Tile U': 1.0, 'Tile V': 0.6, 'Use ORM': 1.0},
                         {'Tint': (0.95, 0.78, 0.62)},
                         {'Base Color': SPRUCE + 'B', 'Normal': SPRUCE + 'N', 'ORM': SPRUCE + 'ORM'}),
        'Cut_Wood': instance(master, 'MI_RawLog_Cut', {'Swap UV': 1.0, 'Tile U': 1.0, 'Tile V': 0.8, 'Roughness': 0.75},
                             {'Tint': (1.45, 1.12, 0.78)}, wood_tex),
        'Hewn_Wood': instance(master, 'MI_RawLog_Hewn', {'Swap UV': 1.0, 'Tile U': 1.0, 'Tile V': 0.8, 'Roughness': 0.8},
                              {'Tint': (1.2, 0.95, 0.68)}, wood_tex),
        'Peeled_Wood': instance(master, 'MI_RawLog_Peeled', {'Swap UV': 1.0, 'Tile U': 1.0, 'Tile V': 0.8, 'Roughness': 0.6},
                                {'Tint': (1.6, 1.3, 0.92)}, wood_tex),
        'Peeled_Cut': instance(master, 'MI_RawLog_PeeledCut', {'Swap UV': 1.0, 'Tile U': 1.0, 'Tile V': 0.8, 'Roughness': 0.7,
                                                                # Peeled: no bark, the slice's bark rim stays outside.
                                                                'End Texture Scale': 0.0235},
                               {'Tint': (1.6, 1.3, 0.92), 'Ring Light': (0.5, 0.33, 0.16), 'Ring Dark': (0.25, 0.14, 0.06)},
                               wood_tex),
    }
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    report = {'master': master.get_path_name(), 'meshes': {}}
    for data in registry.get_assets_by_path('/Game/UnrealExport', recursive=True):
        if str(data.asset_class_path.asset_name) != 'StaticMesh':
            continue
        mesh = unreal.load_asset(str(data.package_name))
        slots = []
        for i, sm in enumerate(mesh.static_materials):
            slot = str(sm.material_slot_name)
            mi = mis.get(slot)
            if mi:
                mesh.set_material(i, mi)
            slots.append([slot, mi.get_name() if mi else None])
        unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
        report['meshes'][str(data.asset_name)] = slots
    with open(OUT, 'w') as f:
        json.dump(report, f, indent=1)
    unreal.log('[RawLogMaterials] {} meshes -> {}'.format(len(report['meshes']), OUT))


main()
