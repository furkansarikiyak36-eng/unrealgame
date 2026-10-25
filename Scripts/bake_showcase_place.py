"""Editor half of the LogShowcase bake: places what Saved/bake_showcase.json lists into /Game/Maps/LogShowcase as static
actors (outliner folder Showcase/Baked; an earlier bake there is removed first), saves the baked mesh assets and the map.
Also keeps the map's own ground: the template floor at z=0, tagged NoDiggableGround (no voxel ground in this map).
Only runs with /Game/Maps/LogShowcase open. -> Saved/bake_showcase_place.json"""
import json, os
import unreal

OUT = os.path.join(unreal.Paths.project_saved_dir(), 'bake_showcase_place.json')
SRC = os.path.join(unreal.Paths.project_saved_dir(), 'bake_showcase.json')
MAP = '/Game/Maps/LogShowcase'
FOLDER = 'Showcase/Baked'
res = {}
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
res['map'] = world.get_outer().get_path_name()


def make_xf(v):
    return unreal.Transform(unreal.Vector(v[0], v[1], v[2]), unreal.Rotator(roll=v[3], pitch=v[4], yaw=v[5]), unreal.Vector(v[6], v[7], v[8]))


if res['map'] != MAP:
    res['error'] = 'open map is not LogShowcase, nothing changed'
else:
    # Without a capture (first step of the bake) this only clears the old bake and sets up the floor.
    data = json.load(open(SRC, encoding='utf-8')) if os.path.exists(SRC) else {'logs': [], 'parts': [], 'signs': []}
    removed = 0
    for a in eas.get_all_level_actors():
        if str(a.get_folder_path()).startswith(FOLDER):
            eas.destroy_actor(a)
            removed += 1
        elif isinstance(a, unreal.StaticMeshActor) and a.get_actor_label() == 'Floor':
            loc = a.get_actor_location()
            if loc.z < -100:
                a.set_actor_location(unreal.Vector(loc.x, loc.y, loc.z + 310.0), False, False)
            tags = list(a.tags)
            if 'NoDiggableGround' not in [str(t) for t in tags]:
                tags.append('NoDiggableGround')
                a.tags = tags
            # Ground for everything placed on it: world static (the template has BlockAllDynamic, which structure
            # support and ground traces do not see).
            a.static_mesh_component.set_collision_profile_name('BlockAll')
            res['floor_z'] = a.get_actor_location().z
        elif isinstance(a, unreal.LogShowcase):
            a.set_editor_property('build_on_play', False)
    res['removed'] = removed

    def place(item, label):
        mesh = unreal.load_asset(item['mesh'])
        if not mesh:
            return False
        t = make_xf(item['xf'])
        actor = eas.spawn_actor_from_class(unreal.StaticMeshActor, t.translation, t.rotation.rotator())
        actor.set_actor_scale3d(t.scale3d)
        comp = actor.static_mesh_component
        comp.set_static_mesh(mesh)
        for i, m in enumerate(item.get('mats', [])):
            if m:
                comp.set_material(i, unreal.load_asset(m))
        actor.set_actor_label(label)
        actor.set_folder_path(FOLDER)
        return True

    res['logs'] = sum(place(it, 'ShowcaseLog_%03d' % i) for i, it in enumerate(data['logs']))
    res['parts'] = sum(place(it, 'ShowcasePart_%03d' % i) for i, it in enumerate(data['parts']))
    signs = 0
    for i, s in enumerate(data['signs']):
        t = make_xf(s['xf'])
        a = eas.spawn_actor_from_class(unreal.TextRenderActor, t.translation, t.rotation.rotator())
        c = a.text_render
        c.set_editor_property('text', s['text'])
        c.set_editor_property('world_size', s['size'])
        c.set_editor_property('horizontal_alignment', unreal.HorizTextAligment.EHTA_CENTER)
        c.set_editor_property('vertical_alignment', unreal.VerticalTextAligment.EVRTA_TEXT_BOTTOM)
        c.set_editor_property('text_render_color', unreal.Color(r=255, g=235, b=190, a=255))
        a.set_actor_label('ShowcaseSign_%02d' % i)
        a.set_folder_path(FOLDER)
        signs += 1
    res['signs'] = signs
    res['assets_saved'] = unreal.EditorAssetLibrary.save_directory('/Game/LogShowcase/Baked', only_if_is_dirty=False, recursive=True)
    res['saved'] = unreal.EditorLoadingAndSavingUtils.save_current_level()

json.dump(res, open(OUT, 'w'), indent=1)
