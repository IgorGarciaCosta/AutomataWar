import unreal


WIDGET_PATH = "/Game/UI/WBP_TableObstableHealth"
OBSTACLE_PATH = "/Game/Blueprints/BP_TableObstable"
RENDERER_PATH = "/Game/Blueprints/BP_AWArenaRenderer"
WIDGET_PARENT_PATH = "/Script/AutomataWar.TableObstableHealthWidget"
OBSTACLE_PARENT_PATH = "/Script/AutomataWar.TableObstable"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
bridge = unreal.AWWidgetBlueprintLibrary


def require_class(path):
    result = unreal.load_class(None, path)
    if not result:
        raise RuntimeError(f"Missing native class: {path}")
    return result


def get_or_create_blueprint(asset_path, parent_path, factory_class):
    result = unreal.load_asset(asset_path)
    if result:
        return result

    package_path, name = asset_path.rsplit("/", 1)
    factory = factory_class()
    factory.set_editor_property("parent_class", require_class(parent_path))
    result = asset_tools.create_asset(name, package_path, None, factory)
    if not result:
        raise RuntimeError(f"Failed to create {asset_path}")
    return result


widget_blueprint = get_or_create_blueprint(
    WIDGET_PATH, WIDGET_PARENT_PATH, unreal.WidgetBlueprintFactory)
widget_tree = bridge.get_widget_tree(widget_blueprint)
if not widget_tree:
    raise RuntimeError("WBP_TableObstableHealth has no WidgetTree")

bridge.clear_widget_tree(widget_tree)
health_bar = bridge.construct_widget(
    widget_tree, unreal.ProgressBar, "HealthBar")
if not health_bar:
    raise RuntimeError("Failed to create HealthBar")
bridge.set_widget_is_variable(health_bar, True)
health_bar.set_percent(1.0)
health_bar.set_fill_color_and_opacity(
    unreal.LinearColor(0.05, 0.85, 0.2, 1.0))
bridge.set_root_widget(widget_tree, health_bar)
unreal.BlueprintEditorLibrary.compile_blueprint(widget_blueprint)
unreal.EditorAssetLibrary.save_loaded_asset(
    widget_blueprint, only_if_is_dirty=False)

obstacle_blueprint = get_or_create_blueprint(
    OBSTACLE_PATH, OBSTACLE_PARENT_PATH, unreal.BlueprintFactory)
unreal.BlueprintEditorLibrary.compile_blueprint(obstacle_blueprint)
obstacle_blueprint.modify()
obstacle_cdo = unreal.get_default_object(obstacle_blueprint.generated_class())
obstacle_cdo.set_editor_property(
    "HealthWidgetClass", widget_blueprint.generated_class())
unreal.BlueprintEditorLibrary.compile_blueprint(obstacle_blueprint)
unreal.EditorAssetLibrary.save_loaded_asset(
    obstacle_blueprint, only_if_is_dirty=False)

renderer_blueprint = unreal.load_asset(RENDERER_PATH)
if not renderer_blueprint:
    raise RuntimeError(f"Missing renderer Blueprint: {RENDERER_PATH}")
renderer_blueprint.modify()
renderer_cdo = unreal.get_default_object(renderer_blueprint.generated_class())
renderer_cdo.set_editor_property(
    "ObstacleClass", obstacle_blueprint.generated_class())
unreal.BlueprintEditorLibrary.compile_blueprint(renderer_blueprint)
unreal.EditorAssetLibrary.save_loaded_asset(
    renderer_blueprint, only_if_is_dirty=False)

unreal.log("AUTOMATA_OBSTACLE_ASSETS_COMPLETE")
