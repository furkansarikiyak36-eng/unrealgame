import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
print("Loading /Game/Maps/TestKitsV2...")
les.load_level("/Game/Maps/TestKitsV2")

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = eas.get_all_level_actors()

logs = [a for a in actors if a and a.get_actor_label().startswith("Log_")]
stones = [a for a in actors if a and a.get_actor_label().startswith("Stone_")]
cobbles = [a for a in actors if a and a.get_actor_label().startswith("Cobblestone_")]
walls = [a for a in actors if a and a.get_actor_label().startswith("StoneWall_")]

print(f"TestKitsV2 Loaded! Total actors: {len(actors)}")
print(f"Counts: Logs={len(logs)}, Stones={len(stones)}, Cobblestone={len(cobbles)}, StoneWall={len(walls)}")

# Floor tag
for a in actors:
    if a and a.get_actor_label() == "Floor":
        tags = [str(t) for t in a.tags]
        if "NoDiggableGround" not in tags:
            a.tags.append("NoDiggableGround")
        if hasattr(a, "static_mesh_component") and a.static_mesh_component:
            a.static_mesh_component.set_collision_profile_name("BlockAll")
        print("Floor NoDiggableGround & BlockAll verified.")
        break

les.save_current_level()
print("TestKitsV2 saved successfully!")
