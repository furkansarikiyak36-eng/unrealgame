"""M_RawLog: fills every texture node still empty with the restored wood texture matching its sampler type (normal
sampler -> wooden_fence_normal, otherwise wooden_fence_albedo), recompiles and saves; then the four MIs are resaved.
py ".../fix_rawlog_samplers.py" -> Saved/fix_rawlog_samplers.json"""
import json, os
import unreal
MEL = unreal.MaterialEditingLibrary; lib = unreal.EditorAssetLibrary
T = '/Game/UnrealExport/Textures/'
alb = unreal.load_asset(T + 'wooden_fence_albedo'); nrm = unreal.load_asset(T + 'wooden_fence_normal')
base = unreal.load_asset('/Game/UnrealExport/Materials/M_RawLog')
rep = {'nodes': []}
for ex in unreal.ObjectIterator(unreal.MaterialExpressionTextureBase):
    if ex.get_outermost() != base.get_outermost():
        continue
    t = ex.get_editor_property('texture'); st = str(ex.get_editor_property('sampler_type'))
    row = [ex.get_name(), ex.get_class().get_name(), st, t.get_name() if t else None]
    if t is None:
        new = nrm if 'NORMAL' in st.upper() else alb
        ex.set_editor_property('texture', new); row.append('-> ' + new.get_name())
    rep['nodes'].append(row)
MEL.recompile_material(base); lib.save_loaded_asset(base, False)
for n in ('Cut', 'Hewn', 'Peeled', 'PeeledCut'):
    mi = unreal.load_asset('/Game/UnrealExport/Materials/MI_RawLog_' + n)
    MEL.update_material_instance(mi); lib.save_loaded_asset(mi, False)
json.dump(rep, open(os.path.join(unreal.Paths.project_saved_dir(), 'fix_rawlog_samplers.json'), 'w'), indent=1)
