"""Megaplant tree actors (tag ShrublandTrees): variant, rotation, bounds, base height, lean (bounds centre offset
from the pivot relative to height), animation/physics state of the skeletal mesh component. Saved/probe_trees.json"""
import json, os
import unreal
out = []
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if 'ShrublandTrees' not in [str(t) for t in a.tags]:
        continue
    c = a.skeletal_mesh_component
    loc = a.get_actor_location()
    org, ext = a.get_actor_bounds(False)
    m = c.get_editor_property('skeletal_mesh_asset') if hasattr(c, 'get_editor_property') else None
    mb = m.get_bounds() if m else None
    out.append({'label': a.get_actor_label(), 'mesh': m.get_name() if m else None,
                'rot': [round(a.get_actor_rotation().pitch, 1), round(a.get_actor_rotation().yaw, 1), round(a.get_actor_rotation().roll, 1)],
                'height_m': round(ext.z * 2 / 100, 1), 'base_z_cm': round(org.z - ext.z - loc.z, 1),
                'lean_xy_m': round(((org.x - loc.x) ** 2 + (org.y - loc.y) ** 2) ** 0.5 / 100, 1),
                'asset_bounds': [round(mb.origin.x), round(mb.origin.y), round(mb.origin.z), round(mb.box_extent.x), round(mb.box_extent.y), round(mb.box_extent.z)] if mb else None,
                'anim_mode': str(c.get_editor_property('animation_mode')),
                'anim_asset': str(c.get_editor_property('animation_data').get_editor_property('anim_to_play')),
                'simulate_physics': c.is_simulating_physics(),
                'physics_asset': str(c.get_editor_property('physics_asset_override')),
                'update_in_editor': c.get_editor_property('update_animation_in_editor')})
json.dump(out, open(os.path.join(unreal.Paths.project_saved_dir(), 'probe_trees.json'), 'w'), indent=1)
