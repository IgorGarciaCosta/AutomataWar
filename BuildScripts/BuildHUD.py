import unreal


HUD_PATH = "/Game/UI/WBP_AWHUD"
SCREEN_DIR = "/Game/UI/Screens"
CONTROLLER_PATH = "/Game/Blueprints/BP_AWPlayerController"
LEGACY_CODE_EDITOR_PATH = "/Game/UI/WBP_AWCodeEditor"
SCREEN_BLUEPRINTS = {
    "MainMenu": ("WBP_AWMainMenuScreen", "/Script/AutomataWar.AWMainMenuScreen"),
    "Programming": ("WBP_AWProgrammingScreen", "/Script/AutomataWar.AWProgrammingScreen"),
    "SimulationDock": ("WBP_AWSimulationDock", "/Script/AutomataWar.AWSimulationDockWidget"),
    "Simulation": ("WBP_AWSimulationScreen", "/Script/AutomataWar.AWSimulationScreen"),
    "ReplayAutopsy": ("WBP_AWReplayAutopsyScreen", "/Script/AutomataWar.AWReplayAutopsyScreen"),
    "ReplayBrowser": ("WBP_AWReplayBrowserScreen", "/Script/AutomataWar.AWReplayBrowserScreen"),
    "LanguageReference": ("WBP_AWLanguageReferenceScreen", "/Script/AutomataWar.AWLanguageReferenceScreen"),
}

TRANSPARENT = unreal.LinearColor(0.0, 0.0, 0.0, 0.0)
BACKGROUND = unreal.LinearColor(0.012, 0.016, 0.022, 1.0)
PANEL = unreal.LinearColor(0.035, 0.045, 0.058, 1.0)
PANEL_ALT = unreal.LinearColor(0.055, 0.065, 0.078, 1.0)
GAMEPLAY_PANEL = unreal.LinearColor(0.025, 0.035, 0.048, 1.0)
TEXT = unreal.LinearColor(0.91, 0.93, 0.96, 1.0)
MUTED = unreal.LinearColor(0.52, 0.58, 0.64, 1.0)
CYAN = unreal.LinearColor(0.0, 0.78, 0.9, 1.0)
CORAL = unreal.LinearColor(0.96, 0.27, 0.22, 1.0)
GREEN = unreal.LinearColor(0.18, 0.72, 0.42, 1.0)
YELLOW = unreal.LinearColor(0.95, 0.68, 0.18, 1.0)
WHITE = unreal.LinearColor(1.0, 1.0, 1.0, 1.0)

H_FILL = unreal.HorizontalAlignment.H_ALIGN_FILL
H_LEFT = unreal.HorizontalAlignment.H_ALIGN_LEFT
H_CENTER = unreal.HorizontalAlignment.H_ALIGN_CENTER
H_RIGHT = unreal.HorizontalAlignment.H_ALIGN_RIGHT
V_FILL = unreal.VerticalAlignment.V_ALIGN_FILL
V_TOP = unreal.VerticalAlignment.V_ALIGN_TOP
V_CENTER = unreal.VerticalAlignment.V_ALIGN_CENTER
V_BOTTOM = unreal.VerticalAlignment.V_ALIGN_BOTTOM

bridge = unreal.AWWidgetBlueprintLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
if unreal.EditorAssetLibrary.does_asset_exist(LEGACY_CODE_EDITOR_PATH):
    unreal.EditorAssetLibrary.delete_asset(LEGACY_CODE_EDITOR_PATH)
hud_blueprint = unreal.load_asset(HUD_PATH)
if not hud_blueprint:
    raise RuntimeError(f"Missing Widget Blueprint: {HUD_PATH}")

hud_tree = bridge.get_widget_tree(hud_blueprint)
if not hud_tree:
    raise RuntimeError("WBP_AWHUD has no WidgetTree")

screen_blueprints = {}
for screen_key, (asset_name, native_class_path) in SCREEN_BLUEPRINTS.items():
    asset_path = f"{SCREEN_DIR}/{asset_name}"
    screen_blueprint = unreal.load_asset(asset_path)
    if not screen_blueprint:
        native_class = unreal.load_class(None, native_class_path)
        if not native_class:
            raise RuntimeError(
                f"Missing native screen class: {native_class_path}")
        factory = unreal.WidgetBlueprintFactory()
        factory.set_editor_property("parent_class", native_class)
        screen_blueprint = asset_tools.create_asset(
            asset_name, SCREEN_DIR, None, factory)
    if not screen_blueprint:
        raise RuntimeError(
            f"Failed to create screen Widget Blueprint: {asset_path}")
    screen_blueprints[screen_key] = screen_blueprint

blueprint = hud_blueprint
tree = hud_tree


def make(widget_class, name, variable=False):
    widget = bridge.construct_widget(tree, widget_class, name)
    if not widget:
        raise RuntimeError(f"Failed to construct {name}")
    if variable:
        bridge.set_widget_is_variable(widget, True)
    return widget


def begin_screen(screen_key):
    global blueprint, tree
    blueprint = screen_blueprints[screen_key]
    tree = bridge.get_widget_tree(blueprint)
    if not tree:
        raise RuntimeError(f"{blueprint.get_name()} has no WidgetTree")
    bridge.clear_widget_tree(tree)


def save_widget_blueprint(widget_key):
    widget_blueprint = screen_blueprints[widget_key]
    widget_blueprint.modify()
    unreal.BlueprintEditorLibrary.compile_blueprint(widget_blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(
        widget_blueprint, only_if_is_dirty=False)


def save_screen(screen_key):
    save_widget_blueprint(screen_key)


def margin(left=0.0, top=0.0, right=0.0, bottom=0.0):
    return unreal.Margin(left=left, top=top, right=right, bottom=bottom)


def child_size(fill=False, value=1.0):
    rule = unreal.SlateSizeRule.FILL if fill else unreal.SlateSizeRule.AUTOMATIC
    return unreal.SlateChildSize(value=value, size_rule=rule)


def add(panel, child, fill=False, padding=None, h=H_FILL, v=V_FILL, weight=1.0):
    slot = panel.add_child(child)
    if padding is not None and hasattr(slot, "set_padding"):
        slot.set_padding(padding)
    if hasattr(slot, "set_horizontal_alignment"):
        slot.set_horizontal_alignment(h)
    if hasattr(slot, "set_vertical_alignment"):
        slot.set_vertical_alignment(v)
    if fill and slot.get_class().get_name() in ["VerticalBoxSlot", "HorizontalBoxSlot"]:
        slot.set_size(child_size(True, weight))
    return slot


def spacer(name, width=0.0, height=0.0):
    item = make(unreal.Spacer, name)
    item.set_editor_property("size", unreal.Vector2D(width, height))
    return item


def slate_color(value):
    return unreal.SlateColor(specified_color=value)


def label(name, value, size=18, color=TEXT, bold=False, mono=False,
          wrap=False, justify=None, variable=False):
    item = make(unreal.TextBlock, name, variable)
    item.set_text(value)
    item.set_color_and_opacity(slate_color(color))
    item.set_auto_wrap_text(wrap)
    font = item.get_editor_property("font")
    font.size = size
    font.letter_spacing = 0
    font.typeface_font_name = "Bold" if bold else "Regular"
    if mono:
        font.force_monospaced = True
    item.set_font(font)
    if justify is not None:
        item.set_editor_property("justification", justify)
    return item


def button(name, value, color=PANEL_ALT, tooltip="", compact=False,
           variable=False):
    item = make(unreal.Button, name, variable)
    item.set_background_color(color)
    item.set_color_and_opacity(WHITE)
    if tooltip:
        item.set_tool_tip_text(tooltip)
    caption = label(f"{name}Label", value, 15 if compact else 18,
                    TEXT, bold=True)
    slot = item.add_child(caption)
    slot.set_padding(
        margin(12, 7 if compact else 10, 12, 7 if compact else 10))
    slot.set_horizontal_alignment(H_CENTER)
    slot.set_vertical_alignment(V_CENTER)
    return item


def input_box(name, hint, variable=True, read_only=False):
    item = make(unreal.EditableTextBox, name, variable)
    item.set_hint_text(hint)
    item.set_is_read_only(read_only)
    item.set_foreground_color(TEXT)
    return item


def panel(name, color=PANEL, padding_value=24.0):
    outer = make(unreal.Border, name)
    outer.set_brush_color(color)
    outer.set_padding(margin(padding_value, padding_value,
                             padding_value, padding_value))
    body = make(unreal.VerticalBox, f"{name}Body")
    outer.add_child(body)
    return outer, body


def frame_rail(name, width=0.0, height=0.0):
    rail_size = make(unreal.SizeBox, name)
    if width > 0.0:
        rail_size.set_width_override(width)
    if height > 0.0:
        rail_size.set_height_override(height)
    rail = make(unreal.Border, f"{name}Fill")
    rail.set_brush_color(PANEL_ALT)
    rail_size.add_child(rail)
    return rail_size


def screen_header(parent, eyebrow, title, subtitle="", accent=CYAN):
    prefix = title.replace(" ", "")
    add(parent, label(f"{prefix}Eyebrow", eyebrow.upper(), 13,
                      accent, bold=True), padding=margin(0, 0, 0, 5))
    add(parent, label(f"{prefix}Title", title.upper(), 32,
                      TEXT, bold=True), padding=margin(0, 0, 0, 4))
    if subtitle:
        add(parent, label(f"{prefix}Subtitle", subtitle, 16,
                          MUTED, wrap=True), padding=margin(0, 0, 0, 16))


begin_screen("MainMenu")
main_screen = make(unreal.Border, "MainMenuScreen")
bridge.set_root_widget(tree, main_screen)
main_screen.set_brush_color(BACKGROUND)
main_screen.set_padding(margin(76, 58, 76, 58))
main_layout = make(unreal.HorizontalBox, "MainMenuLayout")
main_screen.add_child(main_layout)

identity = make(unreal.VerticalBox, "IdentityPanel")
add(main_layout, identity, fill=True, padding=margin(0, 42, 70, 42),
    v=V_CENTER, weight=1.25)
add(identity, label("MainEyebrow", "DETERMINISTIC TACTICAL SIMULATION",
                    15, YELLOW, bold=True), padding=margin(0, 0, 0, 12))
add(identity, label("MainTitle", "AUTOMATA\nWAR", 74, TEXT, bold=True),
    padding=margin(0, 0, 0, 14))
add(identity, label("MainTagline", "PLAN. DEPLOY. OUTTHINK.", 21,
                    CYAN, bold=True), padding=margin(0, 0, 0, 24))
add(identity, label("MainDescription",
                    "EVERY ACTION LEAVES EVIDENCE.",
                    17, MUTED, wrap=True), padding=margin(0, 0, 70, 0))

menu_panel, menu = panel("MainActions", PANEL, 28)
menu_size = make(unreal.SizeBox, "MainActionsSize")
menu_size.set_width_override(540.0)
menu_size.add_child(menu_panel)
add(main_layout, menu_size, padding=margin(0, 10, 0, 10), v=V_CENTER)
add(menu, label("MainActionsTitle", "COMMAND CONSOLE", 20, TEXT, bold=True),
    padding=margin(0, 0, 0, 14))
add(menu, button("LocalMatchButton", "START LOCAL MATCH", CYAN,
                 "Configure both command queues on this machine"),
    padding=margin(0, 0, 0, 10))

lan_row = make(unreal.HorizontalBox, "LanActions")
add(lan_row, button("HostLanButton", "HOST LAN", GREEN,
                    "Create a LAN session", True),
    fill=True, padding=margin(0, 0, 5, 0))
add(lan_row, button("FindLanButton", "FIND LAN", PANEL_ALT,
                    "Search for LAN sessions", True),
    fill=True, padding=margin(5, 0, 0, 0))
add(menu, lan_row, padding=margin(0, 0, 0, 10))

session_combo = make(unreal.ComboBoxString, "SessionComboBox", True)
session_combo.set_tool_tip_text("Discovered LAN sessions")
session_row = make(unreal.HorizontalBox, "SessionRow")
add(session_row, session_combo, fill=True, padding=margin(0, 0, 8, 0),
    v=V_CENTER)
add(session_row, button("JoinSessionButton", "JOIN", GREEN,
                        "Join the selected LAN session", True), v=V_CENTER)
add(menu, session_row, padding=margin(0, 0, 0, 10))

ip_row = make(unreal.HorizontalBox, "DirectIpRow")
add(ip_row, input_box("JoinIPField", "Direct IP address"), fill=True,
    padding=margin(0, 0, 8, 0), v=V_CENTER)
add(ip_row, button("JoinIpButton", "CONNECT", PANEL_ALT,
                   "Connect directly to an IP address", True), v=V_CENTER)
add(menu, ip_row, padding=margin(0, 0, 0, 16))

utility_row = make(unreal.HorizontalBox, "UtilityActions")
add(utility_row, button("ReplayBrowserButton", "REPLAYS", PANEL_ALT,
                        "Browse saved match replays", True),
    fill=True, padding=margin(0, 0, 5, 0))
add(utility_row, button("LanguageReferenceButton", "COMMANDS", PANEL_ALT,
                        "Open the command reference", True),
    fill=True, padding=margin(5, 0, 0, 0))
add(menu, utility_row, padding=margin(0, 0, 0, 10))
add(menu, button("QuitButton", "QUIT", CORAL, "Exit Automata War", True))
save_screen("MainMenu")


begin_screen("Programming")
programming_backdrop = make(unreal.Border, "ProgrammingBackdrop")
bridge.set_root_widget(tree, programming_backdrop)
programming_backdrop.set_brush_color(BACKGROUND)
programming_backdrop.set_padding(margin(28, 24, 28, 24))
programming_screen = make(unreal.VerticalBox, "ProgrammingScreen")
programming_backdrop.add_child(programming_screen)
screen_header(programming_screen, "Match setup", "Programming Bay",
              "TWO COMBATANTS. ONE DETERMINISTIC ARENA.")
programming_nav = make(unreal.HorizontalBox, "ProgrammingNavigation")
add(programming_nav, spacer("ProgrammingNavigationSpace", 0, 0),
    fill=True, weight=1.0)
add(programming_nav, button("ProgrammingBackButton", "BACK TO MENU",
                            PANEL_ALT, "Return to main menu", True), h=H_RIGHT)
add(programming_screen, programming_nav, padding=margin(0, 0, 0, 10))
editors_row = make(unreal.HorizontalBox, "EditorsRow")
add(programming_screen, editors_row, fill=True, weight=1.0)


def build_editor_panel(player_index, accent):
    player_panel, body = panel(f"Player{player_index}Panel", PANEL, 18)
    header = make(unreal.HorizontalBox, f"Player{player_index}Header")
    add(header, label(f"Player{player_index}Title", f"PLAYER {player_index}",
                      22, accent, bold=True), fill=True)
    add(header, label(f"Player{player_index}Slot", f"SLOT {player_index - 1}",
                      13, MUTED, bold=True), h=H_RIGHT, v=V_CENTER)
    add(body, header, padding=margin(0, 0, 0, 10))

    workspace = make(unreal.HorizontalBox, f"Player{player_index}Workspace")

    queue = make(unreal.VerticalBox, f"Player{player_index}Queue")
    queue_header = make(unreal.HorizontalBox, f"Player{player_index}QueueHeader")
    add(queue_header, label(f"Player{player_index}QueueTitle", "ACTION QUEUE",
                            14, MUTED, bold=True), fill=True, v=V_CENTER)
    remove = button(f"RemoveActionP{player_index}Button", "REMOVE ACTION",
                    CORAL, "Remove the last action", True, True)
    remove.set_visibility(unreal.SlateVisibility.COLLAPSED)
    add(queue_header, remove, h=H_RIGHT, v=V_CENTER)
    add(queue, queue_header, padding=margin(0, 0, 0, 8))

    queue_frame = make(unreal.Border, f"Player{player_index}QueueFrame")
    queue_frame.set_brush_color(BACKGROUND)
    queue_frame.set_padding(margin(8, 8, 8, 8))
    queue_scroll = make(unreal.ScrollBox, f"Player{player_index}QueueScroll")
    queue_scroll.set_always_show_scrollbar(True)
    queue_text = label(f"ProgramP{player_index}Text",
                       "NO ACTIONS SELECTED", 14, TEXT,
                       mono=True, variable=True)
    add(queue_scroll, queue_text, padding=margin(4, 4, 12, 4),
        h=H_FILL, v=V_TOP)
    queue_frame.add_child(queue_scroll)
    add(queue, queue_frame, fill=True, weight=1.0)
    add(workspace, queue, fill=True, padding=margin(0, 0, 10, 0), weight=1.0)

    commands_frame = make(unreal.Border, f"Player{player_index}CommandsFrame")
    commands_frame.set_brush_color(GAMEPLAY_PANEL)
    commands_frame.set_padding(margin(10, 10, 10, 10))
    commands = make(unreal.VerticalBox, f"Player{player_index}Commands")
    commands_frame.add_child(commands)
    add(commands, label(f"Player{player_index}CommandsTitle",
                        "AVAILABLE COMMANDS", 14, accent, bold=True,
                        wrap=True), padding=margin(0, 0, 0, 8))
    command_scroll = make(unreal.ScrollBox,
                          f"Player{player_index}CommandsScroll")
    command_buttons = make(unreal.VerticalBox,
                           f"Player{player_index}CommandButtons")
    for name, value in [
            ("Move", "MOVE"),
            ("Fire", "FIRE"),
            ("TurnLeft", "TURN LEFT"),
            ("TurnRight", "TURN RIGHT")]:
        add(command_buttons,
            button(f"{name}P{player_index}Button", value, PANEL_ALT,
                   f"Add {value.lower()} to the queue", True),
            padding=margin(0, 0, 0, 8))
    add(command_scroll, command_buttons, h=H_FILL, v=V_TOP)
    add(commands, command_scroll, fill=True, weight=1.0)
    commands_size = make(unreal.SizeBox, f"Player{player_index}CommandsSize")
    commands_size.set_width_override(190.0)
    commands_size.add_child(commands_frame)
    add(workspace, commands_size)
    add(body, workspace, fill=True, weight=1.0,
        padding=margin(0, 0, 0, 10))

    add(body, button(f"SubmitP{player_index}Button", "SUBMIT", accent,
                     f"Submit player {player_index} command queue", True),
        h=H_FILL)
    return player_panel


add(editors_row, build_editor_panel(1, CYAN), fill=True,
    padding=margin(0, 0, 8, 0), weight=1.0)
add(editors_row, build_editor_panel(2, CORAL), fill=True,
    padding=margin(8, 0, 0, 0), weight=1.0)
save_screen("Programming")


begin_screen("SimulationDock")
simulation_dock = make(unreal.SizeBox, "SimulationDock")
bridge.set_root_widget(tree, simulation_dock)
simulation_dock.set_width_override(360.0)
simulation_dock_panel, simulation_dock_body = panel(
    "SimulationDockPanel", GAMEPLAY_PANEL, 12)
simulation_dock.add_child(simulation_dock_panel)
add(simulation_dock_body, label(
    "SimulationDockTitle", "PLAYER COMMANDS", 16, CYAN,
    bold=True, variable=True), padding=margin(0, 0, 0, 8))

simulation_dock_vertical_scroll = make(
    unreal.ScrollBox, "SimulationDockVerticalScroll")
simulation_dock_vertical_scroll.set_always_show_scrollbar(True)
simulation_dock_horizontal_scroll = make(
    unreal.ScrollBox, "SimulationDockHorizontalScroll")
simulation_dock_horizontal_scroll.set_orientation(
    unreal.Orientation.ORIENT_HORIZONTAL)
simulation_dock_horizontal_scroll.set_always_show_scrollbar(True)
simulation_dock_source = label(
    "SimulationDockCommandsText", "AWAITING COMMANDS", 13, TEXT,
    mono=True, variable=True)
add(simulation_dock_horizontal_scroll, simulation_dock_source,
    padding=margin(8, 8, 8, 8), h=H_LEFT, v=V_TOP)
add(simulation_dock_vertical_scroll, simulation_dock_horizontal_scroll,
    h=H_FILL, v=V_FILL)
add(simulation_dock_body, simulation_dock_vertical_scroll,
    fill=True, weight=1.0, padding=margin(0, 0, 0, 8))
simulation_dock_details = label(
    "SimulationDockDetails", "", 11, CYAN,
    mono=True, wrap=True, variable=True)
simulation_dock_details.set_visibility(unreal.SlateVisibility.COLLAPSED)
add(simulation_dock_body, simulation_dock_details)
save_widget_blueprint("SimulationDock")
simulation_dock_class = screen_blueprints["SimulationDock"].generated_class()


begin_screen("Simulation")
simulation_screen = make(unreal.Border, "SimulationScreen")
bridge.set_root_widget(tree, simulation_screen)
simulation_screen.set_brush_color(TRANSPARENT)
simulation_screen.set_padding(margin(18, 16, 18, 16))
simulation_layout = make(unreal.VerticalBox, "SimulationLayout")
simulation_screen.add_child(simulation_layout)
simulation_banner, simulation_body = panel(
    "SimulationBanner", GAMEPLAY_PANEL, 12)
simulation_header = make(unreal.HorizontalBox, "SimulationHeader")
simulation_titles = make(unreal.VerticalBox, "SimulationTitles")
add(simulation_titles, label("SimulationEyebrow", "MATCH ENGINE", 12,
                             YELLOW, bold=True))
add(simulation_titles, label("SimulationTitle", "TACTICAL EXECUTION", 25,
                             CYAN, bold=True))
add(simulation_header, simulation_titles, fill=True, v=V_CENTER)
add(simulation_header, label("SimulationStatus", "COMMANDS LOCKED", 14,
                             GREEN, bold=True), h=H_RIGHT, v=V_CENTER)
add(simulation_body, simulation_header)
add(simulation_layout, simulation_banner, padding=margin(0, 0, 0, 12))


simulation_workspace = make(unreal.HorizontalBox, "SimulationWorkspace")
simulation_one_dock = make(
    simulation_dock_class, "SimulationP1DockWidget", True)
simulation_one_dock.set_editor_property(
    "PlayerLabel", "PLAYER 1 COMMANDS")
simulation_one_dock.set_editor_property("AccentColor", CYAN)
add(simulation_workspace, simulation_one_dock,
    padding=margin(0, 0, 10, 0))

simulation_arena_frame = make(unreal.Border, "SimulationArenaFrame")
simulation_arena_frame.set_brush_color(TRANSPARENT)
simulation_frame_layout = make(unreal.VerticalBox, "SimulationFrameLayout")
simulation_arena_frame.add_child(simulation_frame_layout)
add(simulation_frame_layout, frame_rail("SimulationFrameTop", height=6.0))
simulation_frame_center = make(unreal.HorizontalBox, "SimulationFrameCenter")
add(simulation_frame_center, frame_rail("SimulationFrameLeft", width=6.0))
simulation_arena_viewport = make(unreal.Border, "SimulationArenaViewport")
simulation_arena_viewport.set_brush_color(TRANSPARENT)
simulation_arena_viewport.add_child(
    spacer("SimulationArenaViewportSpace", 0, 0))
add(simulation_frame_center, simulation_arena_viewport, fill=True, weight=1.0)
add(simulation_frame_center, frame_rail("SimulationFrameRight", width=6.0))
add(simulation_frame_layout, simulation_frame_center, fill=True, weight=1.0)
add(simulation_frame_layout, frame_rail("SimulationFrameBottom", height=6.0))
add(simulation_workspace, simulation_arena_frame, fill=True,
    padding=margin(0, 0, 0, 0), weight=1.0)

simulation_two_dock = make(
    simulation_dock_class, "SimulationP2DockWidget", True)
simulation_two_dock.set_editor_property(
    "PlayerLabel", "PLAYER 2 COMMANDS")
simulation_two_dock.set_editor_property("AccentColor", CORAL)
add(simulation_workspace, simulation_two_dock,
    padding=margin(10, 0, 0, 0))
add(simulation_layout, simulation_workspace, fill=True, weight=1.0,
    padding=margin(0, 0, 0, 12))

simulation_footer, simulation_footer_body = panel(
    "SimulationFooter", GAMEPLAY_PANEL, 10)
simulation_footer_row = make(unreal.HorizontalBox, "SimulationFooterRow")
add(simulation_footer_row, label("SimulationMode", "DETERMINISTIC TIMELINE",
                                 13, YELLOW, bold=True), fill=True)
add(simulation_footer_row, label("SimulationReadout", "RESOLVING MATCH...",
                                 13, MUTED, bold=True), h=H_RIGHT)
add(simulation_footer_body, simulation_footer_row)
add(simulation_layout, simulation_footer)
save_screen("Simulation")


begin_screen("ReplayAutopsy")
replay_backdrop = make(unreal.Border, "ReplayAutopsyBackdrop")
bridge.set_root_widget(tree, replay_backdrop)
replay_backdrop.set_brush_color(TRANSPARENT)
replay_backdrop.set_padding(margin(18, 16, 18, 16))
replay_screen = make(unreal.VerticalBox, "ReplayAutopsyScreen")
replay_backdrop.add_child(replay_screen)
replay_header = make(unreal.HorizontalBox, "ReplayHeader")
replay_titles = make(unreal.VerticalBox, "ReplayTitles")
add(replay_titles, label("ReplayEyebrow", "POST-MATCH ANALYSIS", 13,
                         YELLOW, bold=True))
add(replay_titles, label("ReplayTitle", "REPLAY AUTOPSY", 30,
                         TEXT, bold=True))
add(replay_header, replay_titles, fill=True, v=V_CENTER)
add(replay_header, label("ReplayOutcomeText", "AWAITING REPLAY", 21,
                         CYAN, bold=True, variable=True), h=H_RIGHT, v=V_CENTER)
add(replay_screen, replay_header, padding=margin(0, 0, 0, 12))

transport_panel, transport = panel("ReplayTransport", GAMEPLAY_PANEL, 12)
transport_row = make(unreal.HorizontalBox, "TransportRow")
for name, value, tip in [
        ("ReplayStartButton", "|<", "Jump to start"),
        ("ReplayBackButton", "<", "Step back one action"),
        ("ReplayPauseButton", "||", "Pause playback"),
        ("ReplayPlayButton", ">", "Play replay"),
        ("ReplayStepButton", ">|", "Step forward one action")]:
    add(transport_row, button(name, value, PANEL_ALT, tip, True),
        padding=margin(0, 0, 5, 0), v=V_CENTER)

speed_row = make(unreal.HorizontalBox, "ReplaySpeedControls")
for name, value, tip in [
        ("ReplayQuarterButton", ".25x", "Quarter speed"),
        ("ReplayNormalButton", "1x", "Normal speed"),
        ("ReplayDoubleButton", "2x", "Double speed"),
        ("ReplayQuadButton", "4x", "Quadruple speed")]:
    add(speed_row, button(name, value, PANEL_ALT, tip, True),
        padding=margin(0, 0, 4, 0), v=V_CENTER)
add(transport_row, speed_row, padding=margin(10, 0, 10, 0), v=V_CENTER)

scrub = make(unreal.Slider, "ReplayScrubSlider", True)
scrub.set_slider_bar_color(MUTED)
scrub.set_slider_handle_color(CYAN)
scrub.set_step_size(0.001)
add(transport_row, scrub, fill=True, v=V_CENTER, weight=1.0)
add(transport, transport_row)

timeline_status = make(unreal.HorizontalBox, "TimelineStatus")
add(timeline_status, label("ReplaySpeedText", "SPEED 1x", 14,
                           MUTED, bold=True, variable=True))
add(transport, timeline_status, padding=margin(0, 8, 0, 0))
add(replay_screen, transport_panel, padding=margin(0, 0, 0, 12))

source_row = make(unreal.HorizontalBox, "ReplaySources")

replay_one_dock = make(simulation_dock_class, "ReplayP1DockWidget", True)
replay_one_dock.set_editor_property("PlayerLabel", "PLAYER 1 COMMANDS")
replay_one_dock.set_editor_property("AccentColor", CYAN)
add(source_row, replay_one_dock, padding=margin(0, 0, 10, 0))
replay_arena_frame = make(unreal.Border, "ReplayArenaFrame")
replay_arena_frame.set_brush_color(TRANSPARENT)
replay_frame_layout = make(unreal.VerticalBox, "ReplayFrameLayout")
replay_arena_frame.add_child(replay_frame_layout)
add(replay_frame_layout, frame_rail("ReplayFrameTop", height=6.0))
replay_frame_center = make(unreal.HorizontalBox, "ReplayFrameCenter")
add(replay_frame_center, frame_rail("ReplayFrameLeft", width=6.0))
replay_arena_viewport = make(unreal.Border, "ReplayArenaViewport")
replay_arena_viewport.set_brush_color(TRANSPARENT)
replay_arena_viewport.add_child(spacer("ReplayArenaViewportSpace", 0, 0))
add(replay_frame_center, replay_arena_viewport, fill=True, weight=1.0)
add(replay_frame_center, frame_rail("ReplayFrameRight", width=6.0))
add(replay_frame_layout, replay_frame_center, fill=True, weight=1.0)
add(replay_frame_layout, frame_rail("ReplayFrameBottom", height=6.0))
add(source_row, replay_arena_frame, fill=True, weight=1.0)
replay_two_dock = make(simulation_dock_class, "ReplayP2DockWidget", True)
replay_two_dock.set_editor_property("PlayerLabel", "PLAYER 2 COMMANDS")
replay_two_dock.set_editor_property("AccentColor", CORAL)
add(source_row, replay_two_dock, padding=margin(10, 0, 0, 0))
add(replay_screen, source_row, fill=True, weight=1.0,
    padding=margin(0, 0, 0, 12))

event_panel, event_body = panel(
    "ReplayEventsPanel", GAMEPLAY_PANEL, 10)
event_header = make(unreal.HorizontalBox, "ReplayEventHeader")
add(event_header, label("ReplayEventTitle", "RECENT EVENTS", 14,
                        YELLOW, bold=True), fill=True, v=V_CENTER)
add(event_header, button("ReplayBackToMenuButton", "MENU", PANEL_ALT,
                         "Return to main menu", True), v=V_CENTER)
add(event_header, button("NextRoundButton", "NEXT ROUND", GREEN,
                         "Configure the next round", True),
    padding=margin(8, 0, 0, 0), v=V_CENTER)
add(event_body, event_header, padding=margin(0, 0, 0, 6))
event_scroll = make(unreal.ScrollBox, "ReplayEventVerticalScroll")
event_scroll.set_always_show_scrollbar(True)
event_horizontal_scroll = make(unreal.ScrollBox, "ReplayEventHorizontalScroll")
event_horizontal_scroll.set_orientation(unreal.Orientation.ORIENT_HORIZONTAL)
event_horizontal_scroll.set_always_show_scrollbar(True)
event_text = label("ReplayEventLog", "NO EVENTS", 12, MUTED,
                   mono=True, variable=True)
add(event_horizontal_scroll, event_text, padding=margin(4, 4, 4, 4),
    h=H_LEFT, v=V_TOP)
add(event_scroll, event_horizontal_scroll, h=H_FILL, v=V_FILL)
add(event_body, event_scroll, fill=True, weight=1.0)
event_panel_size = make(unreal.SizeBox, "ReplayEventsDock")
event_panel_size.set_height_override(112.0)
event_panel_size.add_child(event_panel)
add(replay_screen, event_panel_size)
save_screen("ReplayAutopsy")


begin_screen("ReplayBrowser")
browser_backdrop = make(unreal.Border, "ReplayBrowserBackdrop")
bridge.set_root_widget(tree, browser_backdrop)
browser_backdrop.set_brush_color(BACKGROUND)
browser_backdrop.set_padding(margin(32, 28, 32, 28))
browser_screen = make(unreal.VerticalBox, "ReplayBrowserScreen")
browser_backdrop.add_child(browser_screen)
screen_header(browser_screen, "Archive", "Replay Browser",
              "DETERMINISTIC MATCH RECORDS.")
browser_panel, browser_body = panel("ReplayBrowserPanel", PANEL, 24)
browser_toolbar = make(unreal.HorizontalBox, "ReplayBrowserToolbar")
add(browser_toolbar, button("ReplayBrowserBackButton", "BACK", PANEL_ALT,
                            "Return to main menu", True),
    padding=margin(0, 0, 6, 0))
add(browser_toolbar, button("ReplayRefreshButton", "REFRESH", PANEL_ALT,
                            "Refresh saved replays", True),
    padding=margin(0, 0, 6, 0))
add(browser_toolbar, button("ReplaySaveButton", "SAVE CURRENT", GREEN,
                            "Save the current replay", True))
add(browser_body, browser_toolbar, padding=margin(0, 0, 0, 18))

replay_combo = make(unreal.ComboBoxString, "ReplayComboBox", True)
replay_combo.set_tool_tip_text("Saved replay files")
selection_row = make(unreal.HorizontalBox, "ReplaySelectionRow")
add(selection_row, replay_combo, fill=True, padding=margin(0, 0, 8, 0),
    v=V_CENTER)
add(selection_row, button("ReplayLoadButton", "LOAD", CYAN,
                          "Load selected replay", True),
    padding=margin(0, 0, 6, 0), v=V_CENTER)
add(selection_row, button("ReplayExportButton", "EXPORT", PANEL_ALT,
                          "Export selected replay as Base64", True), v=V_CENTER)
add(browser_body, selection_row, padding=margin(0, 0, 0, 16))

add(browser_body, label("ReplayExportLabel", "BASE64 EXPORT", 13,
                        MUTED, bold=True), padding=margin(0, 0, 0, 6))
add(browser_body, input_box("ExportField",
                            "Selected replay export appears here", True, True),
    padding=margin(0, 0, 0, 18))
add(browser_body, label("ReplayImportLabel", "BASE64 IMPORT", 13,
                        MUTED, bold=True), padding=margin(0, 0, 0, 6))
import_row = make(unreal.HorizontalBox, "ReplayImportRow")
add(import_row, input_box("ImportField", "Paste replay Base64 data", True),
    fill=True, padding=margin(0, 0, 8, 0), v=V_CENTER)
add(import_row, button("ReplayImportButton", "IMPORT", YELLOW,
                       "Import the pasted replay", True), v=V_CENTER)
add(browser_body, import_row, padding=margin(0, 0, 0, 18))
add(browser_body, label("ReplayBrowserStatus", "ARCHIVE READY", 14,
                        MUTED, bold=True, variable=True))
add(browser_screen, browser_panel, fill=True, weight=1.0)
save_screen("ReplayBrowser")


begin_screen("LanguageReference")
language_backdrop = make(unreal.Border, "LanguageReferenceBackdrop")
bridge.set_root_widget(tree, language_backdrop)
language_backdrop.set_brush_color(BACKGROUND)
language_backdrop.set_padding(margin(32, 28, 32, 28))
language_screen = make(unreal.VerticalBox, "LanguageReferenceScreen")
language_backdrop.add_child(language_screen)
language_toolbar = make(unreal.HorizontalBox, "LanguageToolbar")
language_titles = make(unreal.VerticalBox, "LanguageTitles")
add(language_titles, label("LanguageEyebrow", "COMMAND MANUAL", 13,
                           YELLOW, bold=True))
add(language_titles, label("LanguageTitle", "COMMAND REFERENCE", 30,
                           TEXT, bold=True))
add(language_toolbar, language_titles, fill=True, v=V_CENTER)
add(language_toolbar, button("LanguageBackButton", "BACK", PANEL_ALT,
                             "Return to main menu", True), h=H_RIGHT, v=V_CENTER)
add(language_screen, language_toolbar, padding=margin(0, 0, 0, 12))
language_panel, language_body = panel("LanguagePanel", PANEL, 20)
language_scroll = make(unreal.ScrollBox, "LanguageScroll")
language_scroll.set_always_show_scrollbar(True)
reference_text = label("LanguageReferenceText", "", 15, TEXT,
                       mono=True, wrap=True, variable=True)
add(language_scroll, reference_text, padding=margin(6, 6, 24, 6),
    h=H_FILL, v=V_TOP)
add(language_body, language_scroll, fill=True, weight=1.0)
add(language_screen, language_panel, fill=True, weight=1.0)
save_screen("LanguageReference")

blueprint = hud_blueprint
tree = hud_tree
bridge.clear_widget_tree(tree)

root = make(unreal.CanvasPanel, "HUDCanvas")
bridge.set_root_widget(tree, root)

background = make(unreal.Border, "HUDBackground")
background.set_brush_color(TRANSPARENT)
background.set_padding(margin())
canvas_slot = root.add_child(background)
canvas_slot.set_anchors(unreal.Anchors(
    minimum=unreal.Vector2D(0.0, 0.0), maximum=unreal.Vector2D(1.0, 1.0)))
canvas_slot.set_offsets(margin())

scale_box = make(unreal.ScaleBox, "ResponsiveScale")
scale_box.set_stretch(unreal.Stretch.SCALE_TO_FIT)
scale_box.set_stretch_direction(unreal.StretchDirection.BOTH)
background.add_child(scale_box)

design_size = make(unreal.SizeBox, "DesignSize")
design_size.set_width_override(1600.0)
design_size.set_height_override(900.0)
scale_box.add_child(design_size)

shell = make(unreal.VerticalBox, "HUDShell")
design_size.add_child(shell)

screen_switcher = make(unreal.WidgetSwitcher, "ScreenSwitcher", True)
add(shell, screen_switcher, fill=True, weight=1.0)
for screen_key, widget_name in [
        ("MainMenu", "MainMenuScreenWidget"),
        ("Programming", "ProgrammingScreenWidget"),
        ("Simulation", "SimulationScreenWidget"),
        ("ReplayAutopsy", "ReplayAutopsyScreenWidget"),
        ("ReplayBrowser", "ReplayBrowserScreenWidget"),
        ("LanguageReference", "LanguageReferenceScreenWidget")]:
    screen_widget = make(
        screen_blueprints[screen_key].generated_class(), widget_name, True)
    screen_switcher.add_child(screen_widget)

status_border = make(unreal.Border, "StatusBar")
status_border.set_brush_color(GAMEPLAY_PANEL)
status_border.set_padding(margin(16, 8, 16, 8))
status_text = label("StatusText", "READY", 14, MUTED, bold=True, variable=True)
status_border.add_child(status_text)
add(shell, status_border, padding=margin(0, 10, 0, 0), v=V_BOTTOM)

screen_switcher.set_active_widget_index(0)
hud_blueprint.modify()
unreal.BlueprintEditorLibrary.compile_blueprint(hud_blueprint)
unreal.EditorAssetLibrary.save_loaded_asset(
    hud_blueprint, only_if_is_dirty=False)

controller = unreal.load_asset(CONTROLLER_PATH)
if not controller:
    raise RuntimeError(f"Missing controller Blueprint: {CONTROLLER_PATH}")
controller_cdo = unreal.get_default_object(controller.generated_class())
controller_cdo.set_editor_property(
    "HUDWidgetClass", hud_blueprint.generated_class())
unreal.EditorAssetLibrary.save_loaded_asset(controller, only_if_is_dirty=False)
unreal.log("AUTOMATA_HUD_COMPLETE")
