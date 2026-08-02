import unreal

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
map_path = "/Game/Maps/L_AutomataArena"
if not unreal.EditorAssetLibrary.does_asset_exist(map_path):
    if not level_editor.new_level(map_path):
        raise RuntimeError(f"Failed to create {map_path}")
else:
    level_editor.load_level(map_path)
if not any(isinstance(actor, unreal.PlayerStart) for actor in actor_editor.get_all_level_actors()):
    actor_editor.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0.0, 0.0, 100.0))
if not level_editor.save_current_level():
    raise RuntimeError(f"Failed to save {map_path}")
unreal.log(f"Created and saved {map_path}")
