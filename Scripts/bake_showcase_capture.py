"""PIE half of the LogShowcase bake: builds every station once with the player's own placement (ALogShowcase::BuildAll),
then turns each log piece (with its cuts and notches) into a static mesh asset under /Game/LogShowcase/Baked
(identical pieces share one asset) and writes what to place where: Saved/bake_showcase.json.
Run in a PIE session of /Game/Maps/LogShowcase; Scripts/bake_log_showcase.sh runs the whole bake."""
import json, os
import unreal

OUT = os.path.join(unreal.Paths.project_saved_dir(), 'bake_showcase.json')
FOLDER = '/Game/LogShowcase/Baked'
res = {'logs': [], 'signs': [], 'parts': [], 'report': None, 'assets': 0}

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()


def xf(t):
    l, r, s = t.translation, t.rotation.rotator(), t.scale3d
    return [l.x, l.y, l.z, r.roll, r.pitch, r.yaw, s.x, s.y, s.z]


def mats(comp):
    return [comp.get_material(i).get_path_name() if comp.get_material(i) else None for i in range(comp.get_num_materials())]


if not world:
    res['error'] = 'no PIE world'
else:
    shows = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.LogShowcase)
    if not shows:
        res['error'] = 'no LogShowcase actor'
    else:
        # A previous bake in the level must not be built on (or captured again): clear it from the play world first.
        cleared = 0
        for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
            if a.get_actor_label().startswith(('ShowcaseLog_', 'ShowcasePart_', 'ShowcaseSign_')):
                a.destroy_actor()
                cleared += 1
        res['cleared_old_bake'] = cleared
        res['report'] = shows[0].build_all()
        opts = unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
        opts.set_editor_property('enable_collision', True)
        try:
            opts.set_editor_property('collision_mode', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        except Exception as e:
            res['collision_note'] = str(e)
        made = {}
        for log in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.CarvableLog):
            comp = log.get_editor_property('mesh_component')
            dm = comp.get_dynamic_mesh()
            box = unreal.GeometryScript_MeshQueries.get_mesh_bounding_box(dm)
            key = '%d_%d_%s' % (unreal.GeometryScript_MeshQueries.get_num_triangle_i_ds(dm),
                                unreal.GeometryScript_MeshQueries.get_vertex_count(dm),
                                '_'.join(str(round(v, 1)) for v in (box.min.x, box.min.y, box.min.z, box.max.x, box.max.y, box.max.z)))
            if key not in made:
                name = '%s/SM_ShowcasePiece_%03d' % (FOLDER, len(made))
                r = unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(dm, name, opts)
                mesh = r[0] if isinstance(r, tuple) else r
                if not mesh:
                    res.setdefault('failed', []).append(name)
                    continue
                made[key] = mesh.get_path_name()
            res['logs'].append({'mesh': made[key], 'xf': xf(comp.get_world_transform()), 'mats': mats(comp)})
        res['assets'] = len(made)
        for t in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TextRenderActor):
            c = t.text_render
            res['signs'].append({'text': str(c.get_editor_property('text')), 'xf': xf(c.get_world_transform()),
                                 'size': c.get_editor_property('world_size')})
        # Carts, sleds and racks: their own static mesh parts.
        for cart in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HaulCart):
            for comp in cart.get_components_by_class(unreal.StaticMeshComponent):
                if comp.static_mesh and comp.is_visible():
                    res['parts'].append({'mesh': comp.static_mesh.get_path_name(), 'xf': xf(comp.get_world_transform()), 'mats': mats(comp)})

json.dump(res, open(OUT, 'w', encoding='utf-8'), indent=1, ensure_ascii=False)
