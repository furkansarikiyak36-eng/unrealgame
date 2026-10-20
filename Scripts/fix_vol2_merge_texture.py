"""vol2's grass_orange_brown_merge was imported as an uncompressed HDR sky texture (4096^2 PF_FloatRGBA, LOD group
Skybox, not streamed: 128 MB). It is a packed mask: compress as masks, World group, streamed (about 21 MB at 4K, and
streamed down when far). Backup: Saved/Backups/2026-10-19/grass_orange_brown_merge.uasset.
Run in the editor's Output Log (Cmd):  py "C:/Users/merts/Downloads/MyProject2/Scripts/fix_vol2_merge_texture.py"
"""
import json, os
import unreal
PATH = '/Game/ProceduralNtr_vol2/pnp_v2_assets/Assets/grass/Textures/grass_orange_brown_merge'
tex = unreal.load_asset(PATH)
before = {'compression': str(tex.get_editor_property('compression_settings')), 'lod_group': str(tex.get_editor_property('lod_group')),
          'srgb': tex.get_editor_property('srgb'), 'never_stream': tex.get_editor_property('never_stream')}
tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS)
tex.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_WORLD)
tex.set_editor_property('srgb', False)
tex.set_editor_property('never_stream', False)
tex.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
saved = unreal.EditorAssetLibrary.save_loaded_asset(tex, False)
after = {'compression': str(tex.get_editor_property('compression_settings')), 'lod_group': str(tex.get_editor_property('lod_group'))}
json.dump({'before': before, 'after': after, 'saved': saved}, open(os.path.join(unreal.Paths.project_saved_dir(), 'fix_vol2_merge_texture.json'), 'w'), indent=1)
unreal.log('[FixVol2Merge] done')
