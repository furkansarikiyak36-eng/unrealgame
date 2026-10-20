"""KitShowcase: groups the mesh actors in the Outliner by role (Saved/mesh_roles.json from classify_mesh_roles.py):
<role>/<kit group>, and tags each actor with its role ("Statik", "Dinamik-kirilir", "Dinamik-toplanir", "Efekt", "Karakter")
and kind. Positions are not changed. py ".../apply_mesh_roles_showcase.py" -> Saved/apply_mesh_roles.json"""
import collections, json, os
import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/KitShowcase.'):
    raise RuntimeError('Open /Game/Maps/KitShowcase first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))
SAVED = unreal.Paths.project_saved_dir()
ROLES = json.load(open(os.path.join(SAVED, 'mesh_roles.json'), encoding='utf-8'))
ASCII = str.maketrans('ıüçşğöİÜÇŞĞÖ', 'iucsgoIUCSGO')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
count = collections.Counter(); missing = []
for a in actors.get_all_level_actors():
    comp = None
    if isinstance(a, unreal.StaticMeshActor):
        m = a.static_mesh_component.get_editor_property('static_mesh')
    elif isinstance(a, unreal.SkeletalMeshActor):
        m = a.skeletal_mesh_component.get_editor_property('skeletal_mesh_asset')
    else:
        continue
    if not m or a.get_actor_label() == 'ShowcaseGround':
        continue
    r = ROLES.get(m.get_outermost().get_name())
    if not r:
        missing.append(a.get_actor_label()); continue
    folder = str(a.get_folder_path())
    for known in ('Statik/', 'Dinamik-kırılır/', 'Dinamik-toplanır/', 'Efekt/', 'Karakter/'):
        if folder.startswith(known):
            folder = folder[len(known):]
    a.set_folder_path(r['role'] + '/' + folder)
    keep = [t for t in a.tags if str(t) not in ('Statik', 'Dinamik-kirilir', 'Dinamik-toplanir', 'Efekt', 'Karakter') and not str(t).startswith('Kind_')]
    a.tags = keep + [unreal.Name(r['role'].translate(ASCII)), unreal.Name('Kind_' + r['kind'].translate(ASCII).replace(' / ', '_').replace(' ', '_'))]
    count[r['role']] += 1
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/KitShowcase')
json.dump({'roles': count, 'missing': missing}, open(os.path.join(SAVED, 'apply_mesh_roles.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
