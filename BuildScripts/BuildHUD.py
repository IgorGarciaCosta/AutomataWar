import os
import struct
import tempfile
import zlib

import unreal


HUD_PATH = "/Game/UI/WBP_AWHUD"
CURSOR_PATH = "/Game/UI/WBP_AWCursor"
CURSOR_TEXTURE_PATH = "/Game/UI/T_AWCursorPixel"
SCREEN_DIR = "/Game/UI/Screens"
CONTROLLER_PATH = "/Game/Blueprints/BP_AWPlayerController"
LEGACY_CODE_EDITOR_PATH = "/Game/UI/WBP_AWCodeEditor"
SCREEN_BLUEPRINTS = {
    "MainMenu": ("WBP_AWMainMenuScreen", "/Script/AutomataWar.AWMainMenuScreen"),
    "Difficulty": ("WBP_AWDifficultyScreen", "/Script/AutomataWar.AWDifficultyScreen"),
    "ProgrammingPanel": ("WBP_AWProgrammingPanel", "/Script/AutomataWar.AWProgrammingPanelWidget"),
    "Programming": ("WBP_AWProgrammingScreen", "/Script/AutomataWar.AWProgrammingScreen"),
    "SimulationDock": ("WBP_AWSimulationDock", "/Script/AutomataWar.AWSimulationDockWidget"),
    "Simulation": ("WBP_AWSimulationScreen", "/Script/AutomataWar.AWSimulationScreen"),
    "ReplayAutopsy": ("WBP_AWReplayAutopsyScreen", "/Script/AutomataWar.AWReplayAutopsyScreen"),
    "ReplayBrowser": ("WBP_AWReplayBrowserScreen", "/Script/AutomataWar.AWReplayBrowserScreen"),
    "LanguageReference": ("WBP_AWLanguageReferenceScreen", "/Script/AutomataWar.AWLanguageReferenceScreen"),
}

TRANSPARENT = unreal.LinearColor(0.0, 0.0, 0.0, 0.0)
DEVICE_CHASSIS = unreal.LinearColor(0.055, 0.06, 0.052, 1.0)
DEVICE_BEZEL = unreal.LinearColor(0.018, 0.021, 0.018, 1.0)
BACKGROUND = unreal.LinearColor(0.002, 0.012, 0.007, 1.0)
PANEL = unreal.LinearColor(0.008, 0.028, 0.017, 1.0)
PANEL_ALT = unreal.LinearColor(0.018, 0.052, 0.032, 1.0)
GAMEPLAY_PANEL = unreal.LinearColor(0.004, 0.021, 0.012, 1.0)
TEXT = unreal.LinearColor(0.72, 1.0, 0.78, 1.0)
MUTED = unreal.LinearColor(0.32, 0.52, 0.36, 1.0)
PHOSPHOR = unreal.LinearColor(0.18, 1.0, 0.42, 1.0)
AMBER = unreal.LinearColor(1.0, 0.62, 0.16, 1.0)
ALARM = unreal.LinearColor(1.0, 0.24, 0.12, 1.0)
FRAME_LINE = unreal.LinearColor(0.08, 0.34, 0.17, 1.0)
GLASS_TINT = unreal.LinearColor(0.0, 0.025, 0.008, 0.18)
SCANLINE = unreal.LinearColor(0.05, 0.2, 0.1, 0.08)
CYAN = PHOSPHOR
CORAL = ALARM
GREEN = unreal.LinearColor(0.34, 0.86, 0.34, 1.0)
YELLOW = AMBER
WHITE = unreal.LinearColor(1.0, 1.0, 1.0, 1.0)

BUTTON_SOUND_PATHS = {
    "navigate": "/Game/Audio/SFX/S_UINavigate",
    "command": "/Game/Audio/SFX/S_UICommand",
    "confirm": "/Game/Audio/SFX/S_UIConfirm",
    "danger": "/Game/Audio/SFX/S_UIDanger",
    "transport": "/Game/Audio/SFX/S_UITransport",
    "hover": "/Game/Audio/SFX/S_UIHover",
}
COMMAND_BUTTONS = {
    "ProgrammingMoveButton", "ProgrammingFireButton",
    "ProgrammingTurnLeftButton", "ProgrammingTurnRightButton",
    "ProgrammingWaitButton", "ProgrammingChargeShieldButton",
    "ProgrammingAccelerateButton"}
CONFIRM_BUTTONS = {
    "SinglePlayerButton", "LocalMatchButton", "HostLanButton", "FindLanButton",
    "DifficultyEasyButton", "DifficultyNormalButton", "DifficultyHardButton",
    "JoinSessionButton", "JoinIpButton", "ProgrammingSubmitButton",
    "ReplaySaveButton", "ReplayLoadButton", "ReplayImportButton",
    "NextRoundButton"}
DANGER_BUTTONS = {"QuitButton", "ProgrammingRemoveActionButton"}
TRANSPORT_BUTTONS = {
    "ReplayStartButton", "ReplayBackButton", "ReplayPauseButton",
    "ReplayPlayButton", "ReplayStepButton", "ReplayQuarterButton",
    "ReplayNormalButton", "ReplayDoubleButton", "ReplayQuadButton"}

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


def png_chunk(chunk_type, data):
    return (struct.pack(">I", len(data)) + chunk_type + data +
            struct.pack(">I", zlib.crc32(chunk_type + data) & 0xffffffff))


def import_cursor_texture():
    logical_width = 15
    logical_height = 20
    scale = 2
    polygon = [
        (0.0, 0.0), (0.0, 16.0), (4.0, 12.0),
        (7.0, 19.0), (10.0, 17.0), (7.0, 11.0), (15.0, 11.0)]

    def inside(column, row):
        x = column + 0.5
        y = row + 0.5
        result = False
        previous = polygon[-1]
        for current in polygon:
            if ((current[1] > y) != (previous[1] > y) and
                    x < (previous[0] - current[0]) *
                    (y - current[1]) /
                    (previous[1] - current[1]) + current[0]):
                result = not result
            previous = current
        return result

    shape = {
        (column, row)
        for row in range(logical_height)
        for column in range(logical_width)
        if inside(column, row)}
    outline = {
        (column, row)
        for column, row in shape
        if any((column + dx, row + dy) not in shape
               for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)))}

    width = logical_width * scale
    height = logical_height * scale
    rows = []
    for pixel_y in range(height):
        row = bytearray([0])
        for pixel_x in range(width):
            cell = (pixel_x // scale, pixel_y // scale)
            if cell in outline:
                row.extend((46, 255, 107, 255))
            elif cell in shape:
                row.extend((0, 0, 0, 255))
            else:
                row.extend((0, 0, 0, 0))
        rows.append(row)

    source_dir = os.path.join(tempfile.gettempdir(), "AutomataWarCursor")
    os.makedirs(source_dir, exist_ok=True)
    source_file = os.path.join(source_dir, "T_AWCursorPixel.png")
    with open(source_file, "wb") as image:
        image.write(b"\x89PNG\r\n\x1a\n")
        image.write(png_chunk(
            b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)))
        image.write(png_chunk(b"IDAT", zlib.compress(b"".join(rows), 9)))
        image.write(png_chunk(b"IEND", b""))

    task = unreal.AssetImportTask()
    task.filename = source_file
    task.destination_path = "/Game/UI"
    task.destination_name = "T_AWCursorPixel"
    task.automated = True
    task.replace_existing = True
    task.save = True
    asset_tools.import_asset_tasks([task])
    texture = unreal.load_asset(CURSOR_TEXTURE_PATH)
    if not texture:
        raise RuntimeError("Failed to import pixel cursor texture")
    texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    texture.set_editor_property(
        "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    unreal.EditorAssetLibrary.save_loaded_asset(
        texture, only_if_is_dirty=False)
    return texture


cursor_texture = import_cursor_texture()
if unreal.EditorAssetLibrary.does_asset_exist(LEGACY_CODE_EDITOR_PATH):
    unreal.EditorAssetLibrary.delete_asset(LEGACY_CODE_EDITOR_PATH)
hud_blueprint = unreal.load_asset(HUD_PATH)
if not hud_blueprint:
    raise RuntimeError(f"Missing Widget Blueprint: {HUD_PATH}")

hud_tree = bridge.get_widget_tree(hud_blueprint)
if not hud_tree:
    raise RuntimeError("WBP_AWHUD has no WidgetTree")

cursor_blueprint = unreal.load_asset(CURSOR_PATH)
if not cursor_blueprint:
    cursor_factory = unreal.WidgetBlueprintFactory()
    cursor_factory.set_editor_property(
        "parent_class", unreal.UserWidget.static_class())
    cursor_blueprint = asset_tools.create_asset(
        "WBP_AWCursor", "/Game/UI", None, cursor_factory)
if not cursor_blueprint:
    raise RuntimeError(f"Failed to create Widget Blueprint: {CURSOR_PATH}")

button_sounds = {}
for profile, asset_path in BUTTON_SOUND_PATHS.items():
    sound = unreal.load_asset(asset_path)
    if not sound:
        raise RuntimeError(
            f"Missing {asset_path}; run BuildScripts/ImportUIAudio.py")
    button_sounds[profile] = sound

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
    font.force_monospaced = True
    item.set_font(font)
    if justify is not None:
        item.set_editor_property("justification", justify)
    return item


def button(name, value, color=PANEL_ALT, tooltip="", compact=False,
           variable=False, caption_size=None):
    item = make(unreal.Button, name, variable)
    item.set_background_color(color)
    item.set_color_and_opacity(WHITE)
    if tooltip:
        item.set_tool_tip_text(tooltip)
    profile = "navigate"
    if name in COMMAND_BUTTONS:
        profile = "command"
    elif name in CONFIRM_BUTTONS:
        profile = "confirm"
    elif name in DANGER_BUTTONS:
        profile = "danger"
    elif name in TRANSPORT_BUTTONS:
        profile = "transport"
    pressed_sound = unreal.SlateSound()
    pressed_sound.set_editor_property(
        "resource_object", button_sounds[profile])
    hovered_sound = unreal.SlateSound()
    hovered_sound.set_editor_property(
        "resource_object", button_sounds["hover"])
    style = item.get_editor_property("widget_style")
    style.set_editor_property("pressed_slate_sound", pressed_sound)
    style.set_editor_property("hovered_slate_sound", hovered_sound)
    item.set_editor_property("widget_style", style)
    caption = label(f"{name}Label", value,
                    caption_size or (15 if compact else 18),
                    TEXT, bold=True)
    slot = item.add_child(caption)
    slot.set_padding(
        margin(12, 7 if compact else 10, 12, 7 if compact else 10))
    slot.set_horizontal_alignment(H_CENTER)
    slot.set_vertical_alignment(V_CENTER)
    return item


def command_button(name, value, cost):
    item = button(name, "", PANEL_ALT,
                  f"Add {value.lower()} to the queue ({cost} AP)", True)
    item.clear_children()
    row = make(unreal.HorizontalBox, f"{name}Content")
    action_size = 10 if len(value) > 10 else 12
    add(row, label(f"{name}Action", value, action_size, TEXT, bold=True),
        fill=True, h=H_LEFT, v=V_CENTER)
    add(row, label(f"{name}Cost", f"{cost} AP", 11, MUTED, bold=True),
        padding=margin(12, 0, 0, 0), h=H_RIGHT, v=V_CENTER)
    slot = item.add_child(row)
    slot.set_padding(margin(10, 7, 10, 7))
    slot.set_horizontal_alignment(H_FILL)
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
    outer.set_brush_color(FRAME_LINE)
    outer.set_padding(margin(1, 1, 1, 1))
    surface = make(unreal.Border, f"{name}Surface")
    surface.set_brush_color(color)
    surface.set_padding(margin(padding_value, padding_value,
                               padding_value, padding_value))
    body = make(unreal.VerticalBox, f"{name}Body")
    surface.add_child(body)
    outer.add_child(surface)
    return outer, body


def frame_rail(name, width=0.0, height=0.0):
    rail_size = make(unreal.SizeBox, name)
    if width > 0.0:
        rail_size.set_width_override(width)
    if height > 0.0:
        rail_size.set_height_override(height)
    rail = make(unreal.Border, f"{name}Fill")
    rail.set_brush_color(FRAME_LINE)
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


blueprint = cursor_blueprint
tree = bridge.get_widget_tree(cursor_blueprint)
if not tree:
    raise RuntimeError("WBP_AWCursor has no WidgetTree")
cursor_root = bridge.get_root_widget(tree)
if not cursor_root:
    bridge.clear_widget_tree(tree)
    cursor_root = make(unreal.SizeBox, "AWCursorRoot")
    bridge.set_root_widget(tree, cursor_root)
cursor_blueprint.modify()
tree.modify()
cursor_root.modify()
cursor_root.set_width_override(30.0)
cursor_root.set_height_override(40.0)
cursor_root.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)


def cursor_widgets(widget, widgets):
    widgets[widget.get_name()] = widget
    if hasattr(widget, "get_all_children"):
        for child in widget.get_all_children():
            cursor_widgets(child, widgets)


cursor_widget_map = {}
cursor_widgets(cursor_root, cursor_widget_map)
cursor_image = cursor_widget_map.get("AWCursorHotspotFill")
if not cursor_image:
    cursor_root.clear_children()
    cursor_image = make(unreal.Border, "AWCursorHotspotFill")
    cursor_root.add_child(cursor_image)
cursor_image.set_brush_from_texture(cursor_texture)
cursor_image.set_brush_color(WHITE)
cursor_image.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)

cursor_hotspot_size = cursor_widget_map.get("AWCursorHotspot")
if cursor_hotspot_size:
    cursor_hotspot_size.set_width_override(30.0)
    cursor_hotspot_size.set_height_override(40.0)
for old_cursor_part in ("AWCursorTopRail", "AWCursorTailRow"):
    if old_cursor_part in cursor_widget_map:
        cursor_widget_map[old_cursor_part].set_visibility(
            unreal.SlateVisibility.COLLAPSED)

cursor_blueprint.modify()
unreal.BlueprintEditorLibrary.compile_blueprint(cursor_blueprint)
unreal.EditorAssetLibrary.save_loaded_asset(
    cursor_blueprint, only_if_is_dirty=False)


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
add(menu, button("SinglePlayerButton", "SINGLE PLAYER", CYAN,
                 "Play against the AI opponent"),
    padding=margin(0, 0, 0, 10))
add(menu, button("LocalMatchButton", "LOCAL VERSUS", PANEL_ALT,
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


begin_screen("Difficulty")
difficulty_backdrop = make(unreal.Border, "DifficultyBackdrop")
bridge.set_root_widget(tree, difficulty_backdrop)
difficulty_backdrop.set_brush_color(BACKGROUND)
difficulty_backdrop.set_padding(margin(180, 90, 180, 90))
difficulty_layout = make(unreal.VerticalBox, "DifficultyLayout")
difficulty_backdrop.add_child(difficulty_layout)
screen_header(difficulty_layout, "Single player", "Select Difficulty",
              "CHOOSE THE AI PLANNING DEPTH.")
difficulty_panel, difficulty_body = panel("DifficultyPanel", PANEL, 28)
for name, title, detail, color in [
        ("Easy", "EASY", "SHORT QUEUES // BASIC MOVEMENT", GREEN),
        ("Normal", "NORMAL", "BALANCED QUEUES // SHIELD USE", CYAN),
        ("Hard", "HARD", "DEEP QUEUES // FULL TACTICAL KIT", CORAL)]:
    row = make(unreal.HorizontalBox, f"Difficulty{name}Row")
    add(row, label(f"Difficulty{name}Detail", detail, 14, MUTED, bold=True),
        fill=True, v=V_CENTER)
    add(row, button(f"Difficulty{name}Button", title, color,
                    f"Start at {title.lower()} difficulty", True),
        h=H_RIGHT, v=V_CENTER)
    add(difficulty_body, row, padding=margin(0, 0, 0, 14))
add(difficulty_body, button("DifficultyBackButton", "BACK", PANEL_ALT,
                            "Return to main menu", True), h=H_LEFT)
add(difficulty_layout, difficulty_panel, fill=True, weight=1.0)
save_screen("Difficulty")


begin_screen("ProgrammingPanel")
programming_panel_root = make(unreal.Overlay, "ProgrammingPanelRoot")
bridge.set_root_widget(tree, programming_panel_root)
programming_panel, programming_panel_body = panel(
    "ProgrammingPanelContent", PANEL, 18)
bridge.set_widget_is_variable(programming_panel, True)
add(programming_panel_root, programming_panel)

programming_panel_header = make(unreal.HorizontalBox, "ProgrammingPanelHeader")
add(programming_panel_header, label(
    "ProgrammingPlayerTitle", "PLAYER 1", 22, CYAN,
    bold=True, variable=True), fill=True)
add(programming_panel_header, label(
    "ProgrammingPlayerStats", "HP 100  |  AP 100", 13, TEXT,
    bold=True, mono=True), padding=margin(0, 0, 18, 0),
    h=H_RIGHT, v=V_CENTER)
add(programming_panel_header, label(
    "ProgrammingPlayerSlot", "SLOT 0", 13, MUTED,
    bold=True, variable=True), h=H_RIGHT, v=V_CENTER)
add(programming_panel_body, programming_panel_header,
    padding=margin(0, 0, 0, 10))

programming_workspace = make(unreal.HorizontalBox, "ProgrammingWorkspace")
programming_queue = make(unreal.VerticalBox, "ProgrammingQueue")
programming_queue_header = make(
    unreal.HorizontalBox, "ProgrammingQueueHeader")
add(programming_queue_header, label(
    "ProgrammingQueueTitle", "ACTION QUEUE", 14, MUTED, bold=True),
    fill=True, v=V_CENTER)
programming_remove = button(
    "ProgrammingRemoveActionButton", "REMOVE ACTION", CORAL,
    "Remove the last action", True, True)
programming_remove.set_visibility(unreal.SlateVisibility.COLLAPSED)
add(programming_queue_header, programming_remove, h=H_RIGHT, v=V_CENTER)
add(programming_queue, programming_queue_header,
    padding=margin(0, 0, 0, 8))

programming_queue_frame = make(unreal.Border, "ProgrammingQueueFrame")
programming_queue_frame.set_brush_color(BACKGROUND)
programming_queue_frame.set_padding(margin(8, 8, 8, 8))
programming_queue_scroll = make(unreal.ScrollBox, "ProgrammingQueueScroll")
programming_queue_scroll.set_always_show_scrollbar(True)
add(programming_queue_scroll, label(
    "ProgrammingProgramText", "NO ACTIONS SELECTED", 14, TEXT,
    mono=True, variable=True), padding=margin(4, 4, 12, 4),
    h=H_FILL, v=V_TOP)
programming_queue_frame.add_child(programming_queue_scroll)
add(programming_queue, programming_queue_frame, fill=True, weight=1.0)
add(programming_workspace, programming_queue, fill=True,
    padding=margin(0, 0, 10, 0), weight=1.0)

programming_commands_frame = make(
    unreal.Border, "ProgrammingCommandsFrame")
programming_commands_frame.set_brush_color(GAMEPLAY_PANEL)
programming_commands_frame.set_padding(margin(10, 10, 10, 10))
programming_commands = make(unreal.VerticalBox, "ProgrammingCommands")
programming_commands_frame.add_child(programming_commands)
add(programming_commands, label(
    "ProgrammingCommandsTitle", "AVAILABLE COMMANDS", 14, CYAN,
    bold=True, wrap=True, variable=True), padding=margin(0, 0, 0, 8))
programming_command_scroll = make(
    unreal.ScrollBox, "ProgrammingCommandsScroll")
programming_command_buttons = make(
    unreal.VerticalBox, "ProgrammingCommandButtons")
for name, value, cost in [
    ("Move", "MOVE", 10),
    ("Fire", "FIRE", 20),
    ("TurnLeft", "TURN LEFT", 5),
    ("TurnRight", "TURN RIGHT", 5),
    ("Wait", "WAIT", 0),
    ("ChargeShield", "CHARGE SHIELD", 20),
        ("Accelerate", "ACCELERATE", 30)]:
    add(programming_command_buttons, command_button(
        f"Programming{name}Button", value, cost),
        padding=margin(0, 0, 0, 8))
add(programming_command_scroll, programming_command_buttons,
    h=H_FILL, v=V_TOP)
add(programming_commands, programming_command_scroll,
    fill=True, weight=1.0)
programming_commands_size = make(unreal.SizeBox, "ProgrammingCommandsSize")
programming_commands_size.set_width_override(290.0)
programming_commands_size.add_child(programming_commands_frame)
add(programming_workspace, programming_commands_size)
add(programming_panel_body, programming_workspace, fill=True, weight=1.0,
    padding=margin(0, 0, 0, 10))
add(programming_panel_body, button(
    "ProgrammingSubmitButton", "SUBMIT", WHITE,
    "Submit this command queue", True, True), h=H_FILL)

programming_return_layer = make(
    unreal.Border, "ProgrammingReturnLayer", True)
programming_return_layer.set_brush_color(GAMEPLAY_PANEL)
programming_return_layer.set_visibility(unreal.SlateVisibility.COLLAPSED)
programming_return_layer.set_padding(margin(32, 32, 32, 32))
programming_return_button = button(
    "ProgrammingReturnToPlanningButton", "RETURN TO PLANNING", CYAN,
    "Withdraw this submission and reopen the command queue", False, True)
add(programming_return_layer, programming_return_button,
    h=H_CENTER, v=V_CENTER)
add(programming_panel_root, programming_return_layer)

programming_shutdown_line = make(
    unreal.SizeBox, "ProgrammingShutdownLine", True)
programming_shutdown_line.set_height_override(4.0)
programming_shutdown_line.set_visibility(unreal.SlateVisibility.COLLAPSED)
programming_shutdown_glow = make(unreal.Border, "ProgrammingShutdownGlow")
programming_shutdown_glow.set_brush_color(WHITE)
programming_shutdown_line.add_child(programming_shutdown_glow)
add(programming_panel_root, programming_shutdown_line,
    h=H_FILL, v=V_CENTER)
save_screen("ProgrammingPanel")


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

programming_panel_class = screen_blueprints["ProgrammingPanel"].generated_class(
)
programming_p1 = make(
    programming_panel_class, "ProgrammingP1PanelWidget", True)
programming_p1.set_editor_property("player_index", 0)
programming_p1.set_editor_property("player_label", "PLAYER 1")
programming_p1.set_editor_property("accent_color", CYAN)
add(editors_row, programming_p1, fill=True,
    padding=margin(0, 0, 8, 0), weight=1.0)

programming_p2 = make(
    programming_panel_class, "ProgrammingP2PanelWidget", True)
programming_p2.set_editor_property("player_index", 1)
programming_p2.set_editor_property("player_label", "PLAYER 2")
programming_p2.set_editor_property("accent_color", AMBER)
add(editors_row, programming_p2, fill=True,
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
simulation_two_dock.set_editor_property("AccentColor", AMBER)
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
replay_two_dock.set_editor_property("AccentColor", AMBER)
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

device_layout = make(unreal.VerticalBox, "DeviceLayout")
design_size.add_child(device_layout)
device_header_frame = make(unreal.Border, "DeviceHeaderFrame")
device_header_frame.set_brush_color(DEVICE_CHASSIS)
device_header_frame.set_padding(margin(30, 8, 30, 8))
device_header = make(unreal.HorizontalBox, "DeviceHeader")
device_header_frame.add_child(device_header)
add(device_header, label(
    "DeviceModel", "AUTOMATA SYSTEMS // MODEL AW-80", 13,
    MUTED, bold=True), fill=True, v=V_CENTER)
add(device_header, label(
    "DeviceTelemetry", "PWR:ON  LINK:LOCAL  TERM:80x24", 12,
    PHOSPHOR, bold=True), h=H_RIGHT, v=V_CENTER)
add(device_layout, device_header_frame)

device_top_bezel_size = make(unreal.SizeBox, "DeviceTopBezelSize")
device_top_bezel_size.set_height_override(14.0)
device_top_bezel = make(unreal.Border, "DeviceTopBezel")
device_top_bezel.set_brush_color(DEVICE_BEZEL)
device_top_bezel_size.add_child(device_top_bezel)
add(device_layout, device_top_bezel_size)

device_middle = make(unreal.HorizontalBox, "DeviceMiddle")
add(device_layout, device_middle, fill=True, weight=1.0)

for rail_name, rail_width, rail_color in [
        ("DeviceLeftChassis", 22.0, DEVICE_CHASSIS),
        ("DeviceLeftBezel", 14.0, DEVICE_BEZEL),
        ("CRTLeftEdge", 2.0, FRAME_LINE)]:
    rail_size = make(unreal.SizeBox, f"{rail_name}Size")
    rail_size.set_width_override(rail_width)
    rail = make(unreal.Border, rail_name)
    rail.set_brush_color(rail_color)
    rail_size.add_child(rail)
    add(device_middle, rail_size)

crt_layers = make(unreal.Overlay, "CRTLayers")
add(device_middle, crt_layers, fill=True, weight=1.0)
glass_tint = make(unreal.Border, "CRTGlassTint")
glass_tint.set_brush_color(GLASS_TINT)
glass_tint.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
add(crt_layers, glass_tint)
shell = make(unreal.VerticalBox, "HUDShell")
add(crt_layers, shell)

for rail_name, rail_width, rail_color in [
        ("CRTRightEdge", 2.0, FRAME_LINE),
        ("DeviceRightBezel", 14.0, DEVICE_BEZEL),
        ("DeviceRightChassis", 22.0, DEVICE_CHASSIS)]:
    rail_size = make(unreal.SizeBox, f"{rail_name}Size")
    rail_size.set_width_override(rail_width)
    rail = make(unreal.Border, rail_name)
    rail.set_brush_color(rail_color)
    rail_size.add_child(rail)
    add(device_middle, rail_size)

screen_switcher = make(unreal.WidgetSwitcher, "ScreenSwitcher", True)
add(shell, screen_switcher, fill=True, weight=1.0)
for screen_key, widget_name in [
        ("MainMenu", "MainMenuScreenWidget"),
    ("Difficulty", "DifficultyScreenWidget"),
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
add(shell, status_border, v=V_BOTTOM)

scanlines = make(unreal.VerticalBox, "CRTScanlines")
scanlines.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
for scanline_index in range(64):
    scanline_size = make(
        unreal.SizeBox, f"CRTScanlineSize{scanline_index}")
    scanline_size.set_height_override(1.0)
    scanline = make(unreal.Border, f"CRTScanline{scanline_index}")
    scanline.set_brush_color(SCANLINE)
    scanline_size.add_child(scanline)
    add(scanlines, scanline_size)
    add(scanlines, spacer(
        f"CRTScanlineGap{scanline_index}", 0.0, 12.0))
add(crt_layers, scanlines)

device_bottom_bezel_size = make(unreal.SizeBox, "DeviceBottomBezelSize")
device_bottom_bezel_size.set_height_override(14.0)
device_bottom_bezel = make(unreal.Border, "DeviceBottomBezel")
device_bottom_bezel.set_brush_color(DEVICE_BEZEL)
device_bottom_bezel_size.add_child(device_bottom_bezel)
add(device_layout, device_bottom_bezel_size)

device_bottom_chassis_size = make(unreal.SizeBox, "DeviceBottomChassisSize")
device_bottom_chassis_size.set_height_override(18.0)
device_bottom_chassis = make(unreal.Border, "DeviceBottomChassis")
device_bottom_chassis.set_brush_color(DEVICE_CHASSIS)
device_bottom_chassis_size.add_child(device_bottom_chassis)
add(device_layout, device_bottom_chassis_size)

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
controller_cdo.set_editor_property(
    "CursorWidgetClass", cursor_blueprint.generated_class())
unreal.EditorAssetLibrary.save_loaded_asset(controller, only_if_is_dirty=False)
unreal.log("AUTOMATA_HUD_COMPLETE")
