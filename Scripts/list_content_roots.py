"""Read only: top-level /Game folders as the editor's asset registry sees them (including unsaved assets),
with asset counts by class. Writes Saved/content_roots.json."""
import collections
import json
import os

import unreal

OUT = os.path.join(unreal.Paths.project_saved_dir(), 'content_roots.json')
registry = unreal.AssetRegistryHelpers.get_asset_registry()
report = {}
for root in registry.get_sub_paths('/Game', False):
    counts = collections.Counter(str(a.asset_class_path.asset_name) for a in registry.get_assets_by_path(root, recursive=True))
    report[str(root)] = dict(counts.most_common(8))
for sub in registry.get_sub_paths('/Game/Fab/Megascans', True):
    report.setdefault('_fab_paths', []).append(str(sub))
dirty = [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
report['_dirty_content'] = dirty[:80]
report['_dirty_maps'] = [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()]
with open(OUT, 'w') as f:
    json.dump(report, f, indent=1)
unreal.log('[ContentRoots] wrote ' + OUT)
