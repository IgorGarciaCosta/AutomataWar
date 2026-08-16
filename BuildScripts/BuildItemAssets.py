import unreal


ITEM_BLUEPRINTS = {
    "/Game/Blueprints/Items/BP_ActionPointItem": "/Script/AutomataWar.AWAPItem",
    "/Game/Blueprints/Items/BP_ExtraAmmoItem": "/Script/AutomataWar.AWExtraAmmoItem",
    "/Game/Blueprints/Items/BP_ShieldItem": "/Script/AutomataWar.AWShieldItem",
    "/Game/Blueprints/Items/BP_AcceleratorItem": "/Script/AutomataWar.AWAcceleratorItem",
}

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


for asset_path, parent_path in ITEM_BLUEPRINTS.items():
    parent_class = unreal.load_class(None, parent_path)
    if not parent_class:
        raise RuntimeError(f"Missing native item class: {parent_path}")

    blueprint = unreal.load_asset(asset_path)
    if not blueprint:
        package_path, asset_name = asset_path.rsplit("/", 1)
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", parent_class)
        blueprint = asset_tools.create_asset(
            asset_name, package_path, unreal.Blueprint, factory)
    if not blueprint:
        raise RuntimeError(f"Failed to create item Blueprint: {asset_path}")

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(
        blueprint, only_if_is_dirty=False)
    native_parent = unreal.EditorAssetLibrary.find_asset_data(
        asset_path).get_tag_value("NativeParentClass")
    if parent_path not in str(native_parent):
        raise RuntimeError(f"{asset_path} has the wrong native parent")

unreal.log(f"AUTOMATA_ITEM_ASSETS_COMPLETE count={len(ITEM_BLUEPRINTS)}")