"""Editor: a temperate-forest climate profile (/Game/RegionSeason/DA_Region_TemperateForest) and one region volume
covering the open PerfTest map, so the season calendar changes temperature, rain and day length there.
py ".../setup_region_season.py" -> Saved/setup_region_season.json. Only runs on /Game/Maps/PerfTest."""
import json, os
import unreal
OUT = os.path.join(unreal.Paths.project_saved_dir(), 'setup_region_season.json')
res = {}
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
path = world.get_outer().get_path_name() if world else ''
res['map'] = path
if path != '/Game/Maps/PerfTest':
    res['error'] = 'open map is not PerfTest, nothing changed'
else:
    ap = '/Game/RegionSeason/DA_Region_TemperateForest'
    prof = unreal.load_asset(ap) if unreal.EditorAssetLibrary.does_asset_exist(ap) else None
    if not prof:
        prof = unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_Region_TemperateForest', '/Game/RegionSeason', unreal.RegionSeasonProfile, unreal.DataAssetFactory())
    prof.set_editor_property('region_id', 'TemperateForest')
    prof.set_editor_property('display_name', unreal.Text('Ilıman orman'))
    prof.set_editor_property('region_type', unreal.WorldRegionType.FOREST)
    prof.set_editor_property('baseline_temperature_c', 9.0)
    prof.set_editor_property('mean_annual_rainfall_mm', 750.0)
    mods = []
    # season, temperature offset C, rain x, wind x, snow threshold offset, daylight h
    for s, t, r, w, sn, d in ((unreal.WorldSeason.SPRING, 1.0, 1.1, 1.1, 0.0, 13.0),
                              (unreal.WorldSeason.SUMMER, 9.0, 0.75, 0.8, 0.0, 15.5),
                              (unreal.WorldSeason.AUTUMN, 0.0, 1.3, 1.3, 0.0, 10.5),
                              (unreal.WorldSeason.WINTER, -12.0, 1.0, 1.4, 2.0, 8.0)):
        m = unreal.RegionSeasonModifier()
        m.set_editor_property('season', s); m.set_editor_property('temperature_offset_c', t)
        m.set_editor_property('precipitation_multiplier', r); m.set_editor_property('wind_multiplier', w)
        m.set_editor_property('snow_threshold_offset_c', sn); m.set_editor_property('daylight_hours', d)
        mods.append(m)
    prof.set_editor_property('seasonal_modifiers', mods)
    unreal.EditorAssetLibrary.save_loaded_asset(prof)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    vols = [a for a in eas.get_all_level_actors() if isinstance(a, unreal.WorldRegionVolume)]
    vol = vols[0] if vols else eas.spawn_actor_from_class(unreal.WorldRegionVolume, unreal.Vector(0, 0, 0))
    vol.set_actor_label('Region_TemperateForest')
    vol.set_editor_property('region_id', 'TemperateForest')
    vol.set_editor_property('region_type', unreal.WorldRegionType.FOREST)
    vol.set_editor_property('climate_profile', prof)
    box = vol.get_editor_property('region_bounds')
    box.set_box_extent(unreal.Vector(200000, 200000, 100000))  # 4 km x 4 km, covers the test map
    res['volume'] = vol.get_actor_label()
    res['profile'] = prof.get_path_name()
    res['saved'] = unreal.EditorLoadingAndSavingUtils.save_current_level()
json.dump(res, open(OUT, 'w'), indent=1)
