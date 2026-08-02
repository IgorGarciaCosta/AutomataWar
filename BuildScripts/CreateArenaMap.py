import unreal

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
map_path = "/Game/Maps/L_AutomataArena"
if not unreal.EditorAssetLibrary.does_asset_exist(map_path):
    if not level_editor.new_level(map_path):
        raise RuntimeError(f"Failed to create {map_path}")
if not level_editor.save_current_level():
    raise RuntimeError(f"Failed to save {map_path}")
unreal.log(f"Created and saved {map_path}")
