"""Imports Saved/AudioSrc/SW_*.wav into /Game/Audio/Felling (replacing), and tags the PerfTest meadow trees (tag
MeadowTree) "Fellable" and meadow rocks (tag MeadowRock) "Breakable" so UTreeFellingSubsystem converts them. Saves the map.
py ".../import_felling_sounds.py" -> Saved/import_felling_sounds.json"""
import glob, json, os
import unreal
_OPEN = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not _OPEN or not _OPEN.get_path_name().startswith('/Game/Maps/PerfTest.'):
    raise RuntimeError('Open /Game/Maps/PerfTest first (saving to it from another map would overwrite it); open map: ' + (_OPEN.get_path_name() if _OPEN else 'none'))
src = os.path.join(unreal.Paths.project_saved_dir(), 'AudioSrc')
tasks = []
for f in sorted(glob.glob(os.path.join(src, 'SW_*.wav'))):
    t = unreal.AssetImportTask(); t.filename = f; t.destination_path = '/Game/Audio/Felling'
    t.automated = True; t.replace_existing = True; t.save = True; tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
rep = {'imported': [p for t in tasks for p in t.imported_object_paths], 'tagged': 0, 'rocks_tagged': 0}
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    tags = [str(x) for x in a.tags]
    if 'MeadowTree' in tags and 'Fellable' not in tags:
        a.tags = list(a.tags) + [unreal.Name('Fellable')]; rep['tagged'] += 1
    if 'MeadowRock' in tags and 'Breakable' not in tags:
        a.tags = list(a.tags) + [unreal.Name('Breakable')]; rep['rocks_tagged'] += 1
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Maps/PerfTest')
json.dump(rep, open(os.path.join(unreal.Paths.project_saved_dir(), 'import_felling_sounds.json'), 'w'), indent=1)
