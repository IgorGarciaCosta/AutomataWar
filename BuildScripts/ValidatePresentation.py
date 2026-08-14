import unreal


HUD_PATH = "/Game/UI/WBP_AWHUD"
CURSOR_PATH = "/Game/UI/WBP_AWCursor"
CURSOR_TEXTURE_PATH = "/Game/UI/T_AWCursorPixel.T_AWCursorPixel"
CONTROLLER_PATH = "/Game/Blueprints/BP_AWPlayerController"
MAP_PATH = "/Game/Maps/L_AutomataArena"
TANK_BLUEPRINT_PATH = "/Game/Blueprints/BP_TankActor"
TANK_CLASS_PATH = "/Game/Blueprints/BP_TankActor.BP_TankActor_C"
SIMULATION_DOCK_PATH = "/Game/UI/Screens/WBP_AWSimulationDock"
SIMULATION_DOCK_CLASS_PATH = "/Game/UI/Screens/WBP_AWSimulationDock.WBP_AWSimulationDock_C"
PROGRAMMING_PANEL_PATH = "/Game/UI/Screens/WBP_AWProgrammingPanel"
PROGRAMMING_PANEL_CLASS_PATH = "/Game/UI/Screens/WBP_AWProgrammingPanel.WBP_AWProgrammingPanel_C"

BUTTON_SOUND_PATHS = {
    "navigate": "/Game/Audio/SFX/S_UINavigate.S_UINavigate",
    "command": "/Game/Audio/SFX/S_UICommand.S_UICommand",
    "confirm": "/Game/Audio/SFX/S_UIConfirm.S_UIConfirm",
    "danger": "/Game/Audio/SFX/S_UIDanger.S_UIDanger",
    "transport": "/Game/Audio/SFX/S_UITransport.S_UITransport",
    "hover": "/Game/Audio/SFX/S_UIHover.S_UIHover",
}
COMMAND_BUTTONS = {
    "ProgrammingMoveButton", "ProgrammingFireButton",
    "ProgrammingTurnLeftButton", "ProgrammingTurnRightButton"}
CONFIRM_BUTTONS = {
    "LocalMatchButton", "HostLanButton", "FindLanButton",
    "JoinSessionButton", "JoinIpButton", "ProgrammingSubmitButton",
    "ReplaySaveButton", "ReplayLoadButton", "ReplayImportButton",
    "NextRoundButton"}
DANGER_BUTTONS = {"QuitButton", "ProgrammingRemoveActionButton"}
TRANSPORT_BUTTONS = {
    "ReplayStartButton", "ReplayBackButton", "ReplayPauseButton",
    "ReplayPlayButton", "ReplayStepButton", "ReplayQuarterButton",
    "ReplayNormalButton", "ReplayDoubleButton", "ReplayQuadButton"}

SCREEN_ASSETS = [
    ("MainMenuScreenWidget", "/Game/UI/Screens/WBP_AWMainMenuScreen", {
        "MainMenuScreen", "JoinIPField", "SessionComboBox",
        "LocalMatchButton", "HostLanButton", "FindLanButton",
        "JoinSessionButton", "JoinIpButton", "ReplayBrowserButton",
        "LanguageReferenceButton", "QuitButton"}),
    ("ProgrammingScreenWidget", "/Game/UI/Screens/WBP_AWProgrammingScreen", {
        "ProgrammingBackdrop", "ProgrammingScreen", "ProgrammingBackButton",
        "EditorsRow", "ProgrammingP1PanelWidget", "ProgrammingP2PanelWidget"}),
    ("SimulationScreenWidget", "/Game/UI/Screens/WBP_AWSimulationScreen", {
        "SimulationScreen", "SimulationBanner", "SimulationP1DockWidget",
        "SimulationP2DockWidget", "SimulationArenaViewport"}),
    ("ReplayAutopsyScreenWidget", "/Game/UI/Screens/WBP_AWReplayAutopsyScreen", {
        "ReplayAutopsyBackdrop", "ReplaySpeedText", "ReplayEventLog",
        "ReplayScrubSlider", "ReplayArenaViewport", "ReplayArenaViewportSpace",
        "ReplayEventsDock", "ReplayP1DockWidget", "ReplayP2DockWidget",
        "ReplayStartButton", "ReplayBackButton",
        "ReplayPauseButton", "ReplayPlayButton", "ReplayStepButton",
        "ReplayQuarterButton", "ReplayNormalButton", "ReplayDoubleButton",
        "ReplayQuadButton",
        "ReplayBackToMenuButton", "NextRoundButton"}),
    ("ReplayBrowserScreenWidget", "/Game/UI/Screens/WBP_AWReplayBrowserScreen", {
        "ReplayBrowserBackdrop", "ReplayComboBox", "ImportField", "ExportField",
        "ReplayBrowserStatus", "ReplayBrowserBackButton", "ReplayRefreshButton",
        "ReplaySaveButton", "ReplayLoadButton", "ReplayExportButton",
        "ReplayImportButton"}),
    ("LanguageReferenceScreenWidget", "/Game/UI/Screens/WBP_AWLanguageReferenceScreen", {
        "LanguageReferenceBackdrop", "LanguageReferenceText", "LanguageBackButton"}),
]
HUD_WIDGETS = {
    "HUDCanvas", "HUDBackground", "ResponsiveScale", "DesignSize",
    "DeviceLayout", "DeviceHeaderFrame", "DeviceHeader", "DeviceModel",
    "DeviceTelemetry", "DeviceMiddle", "CRTLayers", "CRTGlassTint",
    "CRTScanlines", "ScreenSwitcher", "StatusBar", "StatusText",
    *(entry[0] for entry in SCREEN_ASSETS)
}

hud = unreal.load_asset(HUD_PATH)
if not hud:
    raise RuntimeError(f"Missing {HUD_PATH}")

tree = unreal.AWWidgetBlueprintLibrary.get_widget_tree(hud)
root = unreal.AWWidgetBlueprintLibrary.get_root_widget(tree)
if not root or root.get_class().get_name() != "CanvasPanel":
    raise RuntimeError("WBP_AWHUD root must be a CanvasPanel")


def collect_widgets(widget, widgets):
    widgets[widget.get_name()] = widget
    if hasattr(widget, "get_all_children"):
        for child in widget.get_all_children():
            collect_widgets(child, widgets)


def sound_path(button, property_name):
    style = button.get_editor_property("widget_style")
    sound = style.get_editor_property(property_name)
    resource = sound.get_editor_property("resource_object")
    return resource.get_path_name() if resource else ""


cursor = unreal.load_asset(CURSOR_PATH)
if not cursor:
    raise RuntimeError(f"Missing custom software cursor: {CURSOR_PATH}")
unreal.BlueprintEditorLibrary.compile_blueprint(cursor)
cursor_tree = unreal.AWWidgetBlueprintLibrary.get_widget_tree(cursor)
cursor_root = unreal.AWWidgetBlueprintLibrary.get_root_widget(cursor_tree)
cursor_widgets = {}
collect_widgets(cursor_root, cursor_widgets)
required_cursor_widgets = {
    "AWCursorRoot", "AWCursorHotspotFill"}
missing = sorted(required_cursor_widgets - set(cursor_widgets))
if missing:
    raise RuntimeError(
        f"{CURSOR_PATH} is missing widgets: {missing}; "
        f"found: {sorted(cursor_widgets)}")

if cursor_root.get_editor_property("width_override") != 30.0 or \
        cursor_root.get_editor_property("height_override") != 40.0:
    raise RuntimeError("Custom cursor must retain its 30x40 pixel size")

cursor_brush = cursor_widgets["AWCursorHotspotFill"].get_editor_property(
    "background")
cursor_resource = cursor_brush.get_editor_property("resource_object")
if not cursor_resource or cursor_resource.get_path_name() != CURSOR_TEXTURE_PATH:
    raise RuntimeError("Custom cursor does not use the pixel arrow texture")
if cursor_resource.get_editor_property("filter") != unreal.TextureFilter.TF_NEAREST:
    raise RuntimeError("Custom cursor texture must use nearest filtering")
if cursor_resource.get_editor_property("mip_gen_settings") != unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS:
    raise RuntimeError("Custom cursor texture must not generate mipmaps")
for old_cursor_part in ("AWCursorTopRail", "AWCursorTailRow"):
    if old_cursor_part in cursor_widgets and cursor_widgets[old_cursor_part].get_visibility() != unreal.SlateVisibility.COLLAPSED:
        raise RuntimeError(f"Legacy cursor part remains visible: {old_cursor_part}")


hud_widgets = {}
collect_widgets(root, hud_widgets)
missing = sorted(HUD_WIDGETS - set(hud_widgets))
if missing:
    raise RuntimeError(f"Missing required HUD widgets: {missing}")

hud_background = hud_widgets["HUDBackground"]
if hud_background.get_editor_property("brush_color").a > 0.01:
    raise RuntimeError("HUDBackground must remain transparent over the arena")

glass_alpha = hud_widgets["CRTGlassTint"].get_editor_property("brush_color").a
if glass_alpha <= 0.0 or glass_alpha > 0.25:
    raise RuntimeError(
        "CRT glass tint must be visible without hiding the arena")

missing_scanlines = [
    name for name in (f"CRTScanline{index}" for index in range(64))
    if name not in hud_widgets]
if missing_scanlines:
    raise RuntimeError(
        f"CRT scanline field is incomplete: {missing_scanlines}")

switcher = hud_widgets["ScreenSwitcher"]
if switcher.get_num_widgets() != 6:
    raise RuntimeError(
        f"ScreenSwitcher has {switcher.get_num_widgets()} screens instead of 6")

programming_panel = unreal.load_asset(PROGRAMMING_PANEL_PATH)
if not programming_panel:
    raise RuntimeError(
        f"Missing reusable programming panel: {PROGRAMMING_PANEL_PATH}")
unreal.BlueprintEditorLibrary.compile_blueprint(programming_panel)
programming_panel_tree = unreal.AWWidgetBlueprintLibrary.get_widget_tree(
    programming_panel)
programming_panel_root = unreal.AWWidgetBlueprintLibrary.get_root_widget(
    programming_panel_tree)
programming_panel_widgets = {}
collect_widgets(programming_panel_root, programming_panel_widgets)
required_programming_panel_widgets = {
    "ProgrammingPanelRoot", "ProgrammingPanelContent",
    "ProgrammingReturnLayer", "ProgrammingShutdownLine",
    "ProgrammingPlayerTitle", "ProgrammingPlayerSlot",
    "ProgrammingCommandsTitle", "ProgrammingProgramText",
    "ProgrammingRemoveActionButton", "ProgrammingMoveButton",
    "ProgrammingFireButton", "ProgrammingTurnLeftButton",
    "ProgrammingTurnRightButton", "ProgrammingSubmitButton",
    "ProgrammingReturnToPlanningButton"}
missing = sorted(required_programming_panel_widgets -
                 set(programming_panel_widgets))
if missing:
    raise RuntimeError(
        f"{PROGRAMMING_PANEL_PATH} is missing widgets: {missing}")

simulation_dock = unreal.load_asset(SIMULATION_DOCK_PATH)
if not simulation_dock:
    raise RuntimeError(
        f"Missing reusable simulation dock: {SIMULATION_DOCK_PATH}")
unreal.BlueprintEditorLibrary.compile_blueprint(simulation_dock)
simulation_dock_tree = unreal.AWWidgetBlueprintLibrary.get_widget_tree(
    simulation_dock)
simulation_dock_root = unreal.AWWidgetBlueprintLibrary.get_root_widget(
    simulation_dock_tree)
simulation_dock_widgets = {}
collect_widgets(simulation_dock_root, simulation_dock_widgets)
required_dock_widgets = {
    "SimulationDock", "SimulationDockTitle", "SimulationDockCommandsText",
    "SimulationDockDetails", "SimulationDockVerticalScroll",
    "SimulationDockHorizontalScroll"}
missing = sorted(required_dock_widgets - set(simulation_dock_widgets))
if missing:
    raise RuntimeError(f"{SIMULATION_DOCK_PATH} is missing widgets: {missing}")

screen_widgets = {}
for index, (widget_name, asset_path, required_widgets) in enumerate(SCREEN_ASSETS):
    screen = unreal.load_asset(asset_path)
    if not screen:
        raise RuntimeError(f"Missing modular screen asset: {asset_path}")
    unreal.BlueprintEditorLibrary.compile_blueprint(screen)

    child = switcher.get_child_at(index)
    expected_class = f"{asset_path}.{asset_path.rsplit('/', 1)[1]}_C"
    if child.get_name() != widget_name or child.get_class().get_path_name() != expected_class:
        raise RuntimeError(
            f"Screen {index} must be {widget_name} ({expected_class})")

    screen_tree = unreal.AWWidgetBlueprintLibrary.get_widget_tree(screen)
    screen_root = unreal.AWWidgetBlueprintLibrary.get_root_widget(screen_tree)
    widgets = {}
    collect_widgets(screen_root, widgets)
    missing = sorted(required_widgets - set(widgets))
    if missing:
        raise RuntimeError(f"{asset_path} is missing widgets: {missing}")
    screen_widgets.update(widgets)

for name in ["SimulationP1DockWidget", "SimulationP2DockWidget"]:
    if screen_widgets[name].get_class().get_path_name() != SIMULATION_DOCK_CLASS_PATH:
        raise RuntimeError(f"{name} must use the reusable simulation dock WBP")

for name, expected_index in [
        ("ProgrammingP1PanelWidget", 0),
        ("ProgrammingP2PanelWidget", 1)]:
    panel_widget = screen_widgets[name]
    if panel_widget.get_class().get_path_name() != PROGRAMMING_PANEL_CLASS_PATH:
        raise RuntimeError(
            f"{name} must use the reusable programming panel WBP")
    if panel_widget.get_editor_property("player_index") != expected_index:
        raise RuntimeError(f"{name} has the wrong player index")

if simulation_dock_widgets["SimulationDockCommandsText"].get_class().get_name() != "TextBlock":
    raise RuntimeError(
        "Simulation command output must remain a read-only TextBlock")
screen_widgets.update(programming_panel_widgets)
screen_widgets.update(simulation_dock_widgets)

button_profiles = set()
for name, widget in screen_widgets.items():
    if widget.get_class().get_name() != "Button":
        continue

    profile = "navigate"
    if name in COMMAND_BUTTONS:
        profile = "command"
    elif name in CONFIRM_BUTTONS:
        profile = "confirm"
    elif name in DANGER_BUTTONS:
        profile = "danger"
    elif name in TRANSPORT_BUTTONS:
        profile = "transport"
    button_profiles.add(profile)

    if sound_path(widget, "pressed_slate_sound") != BUTTON_SOUND_PATHS[profile]:
        raise RuntimeError(f"{name} has the wrong {profile} pressed sound")
    if sound_path(widget, "hovered_slate_sound") != BUTTON_SOUND_PATHS["hover"]:
        raise RuntimeError(f"{name} has the wrong hover sound")

if button_profiles != {"navigate", "command", "confirm", "danger", "transport"}:
    raise RuntimeError(f"Missing button sound profiles: {button_profiles}")

internal_widgets = set(screen_widgets) - HUD_WIDGETS
if internal_widgets & set(hud_widgets):
    raise RuntimeError("WBP_AWHUD still owns internal screen controls")

for name in ["MainMenuScreen", "ProgrammingBackdrop",
             "ReplayBrowserBackdrop", "LanguageReferenceBackdrop"]:
    if screen_widgets[name].get_editor_property("brush_color").a < 0.99:
        raise RuntimeError(
            f"{name} must hide the arena with an opaque background")

for name in ["SimulationArenaViewport", "ReplayArenaViewport"]:
    if screen_widgets[name].get_editor_property("brush_color").a > 0.01:
        raise RuntimeError(
            f"{name} must remain transparent for the arena view")

if screen_widgets["ReplayEventsDock"].get_editor_property("height_override") != 112.0:
    raise RuntimeError("ReplayEventsDock must keep a fixed height")

unreal.BlueprintEditorLibrary.compile_blueprint(hud)

tank_blueprint = unreal.load_asset(TANK_BLUEPRINT_PATH)
if not tank_blueprint:
    raise RuntimeError("Missing BP_TankActor")
unreal.BlueprintEditorLibrary.compile_blueprint(tank_blueprint)
if tank_blueprint.generated_class().get_path_name() != TANK_CLASS_PATH:
    raise RuntimeError("BP_TankActor generated class is invalid")

controller = unreal.load_asset(CONTROLLER_PATH)
controller_cdo = unreal.get_default_object(controller.generated_class())
hud_class = controller_cdo.get_editor_property("HUDWidgetClass")
if hud_class.get_path_name() != "/Game/UI/WBP_AWHUD.WBP_AWHUD_C":
    raise RuntimeError("BP_AWPlayerController does not instantiate WBP_AWHUD")
cursor_class = controller_cdo.get_editor_property("CursorWidgetClass")
if cursor_class.get_path_name() != "/Game/UI/WBP_AWCursor.WBP_AWCursor_C":
    raise RuntimeError("BP_AWPlayerController does not use WBP_AWCursor")

if unreal.EditorAssetLibrary.does_asset_exist("/Game/Audio/SFX/S_Move"):
    raise RuntimeError("Obsolete tank movement sound is still present")

actor_editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
    raise RuntimeError(f"Failed to load {MAP_PATH}")

actors = actor_editor.get_all_level_actors()
by_label = {actor.get_actor_label(): actor for actor in actors}
tank_one = by_label.get("Tank_PlayerOne")
tank_two = by_label.get("Tank_PlayerTwo")
renderer = by_label.get("BP_AWArenaRenderer")
camera_actor = by_label.get("BP_AWIsometricCamera")
if not tank_one or not tank_two or not renderer or not camera_actor:
    raise RuntimeError("Arena is missing renderer, camera, or tank actors")

camera_components = camera_actor.get_components_by_class(
    unreal.CameraComponent)
if len(camera_components) != 1:
    raise RuntimeError("Arena camera must have exactly one CameraComponent")
if camera_components[0].get_editor_property("projection_mode") != unreal.CameraProjectionMode.ORTHOGRAPHIC:
    raise RuntimeError("Arena camera must use orthographic projection")

for tank, expected_index in [(tank_one, 0), (tank_two, 1)]:
    if tank.get_class().get_path_name() != TANK_CLASS_PATH:
        raise RuntimeError(
            f"{tank.get_actor_label()} is not an instance of BP_TankActor")
    if tank.get_editor_property("RobotIndex") != expected_index:
        raise RuntimeError(
            f"{tank.get_actor_label()} has the wrong RobotIndex")
    if not tank.get_editor_property("TankAsset"):
        raise RuntimeError(f"{tank.get_actor_label()} has no mesh")
    cannon = tank.get_editor_property("CannonAsset")
    if not cannon:
        raise RuntimeError(f"{tank.get_actor_label()} has no cannon mesh")
    if not cannon.find_socket("Muzzle"):
        raise RuntimeError(
            f"{tank.get_actor_label()} cannon has no Muzzle socket")

if renderer.get_editor_property("PlayerOneTank") != tank_one:
    raise RuntimeError("Renderer PlayerOneTank reference is invalid")
if renderer.get_editor_property("PlayerTwoTank") != tank_two:
    raise RuntimeError("Renderer PlayerTwoTank reference is invalid")

wall_labels = [label for label in by_label
               if label.startswith("Arena_Rail_") or label.startswith("ServiceWall_")]
if len(wall_labels) < 8:
    raise RuntimeError("Expected level-authored arena walls are missing")

unreal.log(
    f"AUTOMATA_PRESENTATION_VALIDATION_COMPLETE widgets={len(hud_widgets) + len(screen_widgets) + len(cursor_widgets)} "
    f"screens=6 cursor=1 button_profiles=5 cameras=1 tanks=2 walls={len(wall_labels)}")
