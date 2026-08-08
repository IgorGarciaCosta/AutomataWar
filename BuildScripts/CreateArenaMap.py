import unreal


MAP_PATH = "/Game/Maps/L_AutomataArena"
BLUEPRINT_DIR = "/Game/Blueprints"
UI_DIR = "/Game/UI"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def require_asset(path):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError(f"Required asset is missing: {path}")
    return asset


def require_class(path):
    loaded_class = unreal.load_class(None, path)
    if not loaded_class:
        raise RuntimeError(f"Required class is missing: {path}")
    return loaded_class


def get_or_create_blueprint(name, destination, parent_path, factory_class):
    asset_path = f"{destination}/{name}"
    parent_class = require_class(parent_path)
    blueprint = unreal.load_asset(asset_path)
    if not blueprint:
        factory = factory_class()
        factory.set_editor_property("parent_class", parent_class)
        blueprint = asset_tools.create_asset(name, destination, None, factory)
    if not blueprint:
        raise RuntimeError(f"Failed to create {asset_path}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
    return blueprint


def generated_class(blueprint):
    result = blueprint.generated_class()
    if not result:
        raise RuntimeError(
            f"Blueprint has no generated class: {blueprint.get_path_name()}")
    return result


def configure_blueprints():
    hud = get_or_create_blueprint(
        "WBP_AWHUD", UI_DIR, "/Script/AutomataWar.AWHUDWidget",
        unreal.WidgetBlueprintFactory)
    code_editor = get_or_create_blueprint(
        "WBP_AWCodeEditor", UI_DIR, "/Script/AutomataWar.AWCodeEditorWidget",
        unreal.WidgetBlueprintFactory)
    player = get_or_create_blueprint(
        "BP_AWPlayer", BLUEPRINT_DIR, "/Script/AutomataWar.AWSpectatorPawn",
        unreal.BlueprintFactory)
    controller = get_or_create_blueprint(
        "BP_AWPlayerController", BLUEPRINT_DIR,
        "/Script/AutomataWar.AWPlayerController", unreal.BlueprintFactory)
    game_mode = get_or_create_blueprint(
        "BP_AWGameMode", BLUEPRINT_DIR, "/Script/AutomataWar.AWGameMode",
        unreal.BlueprintFactory)
    camera = get_or_create_blueprint(
        "BP_AWIsometricCamera", BLUEPRINT_DIR,
        "/Script/AutomataWar.AWIsometricCamera", unreal.BlueprintFactory)
    renderer = get_or_create_blueprint(
        "BP_AWArenaRenderer", BLUEPRINT_DIR,
        "/Script/AutomataWar.AWArenaRenderer", unreal.BlueprintFactory)
    tank = get_or_create_blueprint(
        "BP_TankActor", BLUEPRINT_DIR,
        "/Script/AutomataWar.AWTankActor", unreal.BlueprintFactory)

    controller_cdo = unreal.get_default_object(generated_class(controller))
    try:
        controller_cdo.set_editor_property(
            "HUDWidgetClass", generated_class(hud))
    except Exception:
        unreal.log_warning(
            "Live Coding prevented WBP_AWHUD assignment; preserving inherited native HUD")
    unreal.EditorAssetLibrary.save_loaded_asset(controller)

    game_mode_cdo = unreal.get_default_object(generated_class(game_mode))
    game_mode_cdo.set_editor_property(
        "PlayerControllerClass", generated_class(controller))
    game_mode_cdo.set_editor_property(
        "DefaultPawnClass", generated_class(player))
    unreal.EditorAssetLibrary.save_loaded_asset(game_mode)

    return {
        "hud": hud,
        "code_editor": code_editor,
        "player": player,
        "controller": controller,
        "game_mode": game_mode,
        "camera": camera,
        "renderer": renderer,
        "tank": tank,
    }


def spawn(actor_class, label, location, rotation=None, scale=None):
    actor = actor_editor.spawn_actor_from_class(
        actor_class, unreal.Vector(*location))
    if not actor:
        raise RuntimeError(f"Failed to spawn {label}")
    actor.set_actor_label(label)
    if rotation:
        actor.set_actor_rotation(unreal.Rotator(
            pitch=rotation[0], yaw=rotation[1], roll=rotation[2]), False)
    if scale:
        actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor


def static_mesh_actor(label, mesh_path, location, scale, material_map, rotation=(0, 0, 0)):
    actor = spawn(unreal.StaticMeshActor, label, location, rotation, scale)
    component = actor.get_editor_property("static_mesh_component")
    mesh = require_asset(mesh_path)
    component.set_static_mesh(mesh)
    component.set_mobility(unreal.ComponentMobility.STATIC)

    slots = mesh.get_editor_property("static_materials")
    fallback = material_map.get("*")
    for index, slot in enumerate(slots):
        slot_name = str(slot.get_editor_property("material_slot_name"))
        material = material_map.get(slot_name, fallback)
        if material:
            component.set_material(index, material)
    return actor


def set_optional(target, property_name, value):
    try:
        target.set_editor_property(property_name, value)
    except Exception as error:
        unreal.log_warning(f"Skipped optional {property_name}: {error}")


# ═══════════════════════════════════════════════════════════════════════════════
# Level construction — each helper owns one visual/logical concern.
# ═══════════════════════════════════════════════════════════════════════════════


def _load_materials():
    """Load the shared material palette used across floor and dressing."""
    return {
        "ground": require_asset("/Game/Art/Materials/M_Ground"),
        "foliage": require_asset("/Game/Art/Materials/M_Foliage"),
        "wood": require_asset("/Game/Art/Materials/M_Wood"),
        "stone": require_asset("/Game/Art/Materials/M_Stone"),
        "industrial": require_asset("/Game/Art/Materials/M_Industrial"),
    }


def _create_or_load_level():
    """Open the arena map if it exists, otherwise create it fresh and wipe all actors."""
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        level_editor.load_level(MAP_PATH)
    elif not level_editor.new_level(MAP_PATH):
        raise RuntimeError(f"Failed to create {MAP_PATH}")

    for actor in actor_editor.get_all_level_actors():
        actor_editor.destroy_actor(actor)


def _build_floor_geometry(materials):
    """Spawn the arena ground plane, 16x16 grid lines, and perimeter rails."""
    cube = "/Engine/BasicShapes/Cube"

    # Base ground slab and raised foundation matching the 16-cell play area.
    static_mesh_actor(
        "Arena_Ground", cube, (800, 800, -75), (32, 32, 1),
        {"*": materials["stone"]})
    static_mesh_actor(
        "Arena_Foundation", cube, (800, 800, -12.5), (18, 18, 0.25),
        {"*": materials["ground"]})

    # Thin cubes forming the 17 vertical and 17 horizontal grid lines.
    for index in range(17):
        coordinate = index * 100
        static_mesh_actor(
            f"GridLine_X_{index:02}", cube, (coordinate, 800, 0.2),
            (0.015, 16, 0.004), {"*": materials["industrial"]})
        static_mesh_actor(
            f"GridLine_Y_{index:02}", cube, (800, coordinate, 0.2),
            (16, 0.015, 0.004), {"*": materials["industrial"]})

    # Border rails enclosing the play area on all four sides.
    rails = [
        ("Arena_Rail_South", (800, -75, 35), (18, 0.25, 0.7)),
        ("Arena_Rail_North", (800, 1675, 35), (18, 0.25, 0.7)),
        ("Arena_Rail_West", (-75, 800, 35), (0.25, 18, 0.7)),
        ("Arena_Rail_East", (1675, 800, 35), (0.25, 18, 0.7)),
    ]
    for label, location, scale in rails:
        static_mesh_actor(
            label, cube, location, scale, {"*": materials["industrial"]})


def _build_environment_dressing(materials):
    """Place decorative geometry outside the play area: walls, barrels, trees, rocks."""
    cube = "/Engine/BasicShapes/Cube"
    cylinder = "/Engine/BasicShapes/Cylinder"

    # Service walls give depth to the scene perimeter.
    walls = [
        ("ServiceWall_West_A", (-360, 600, 50), (0.35, 3.2, 1.5), 8),
        ("ServiceWall_West_B", (-280, 1060, 35), (1.8, 0.3, 1.2), -12),
        ("ServiceWall_East_A", (1970, 1260, 45), (0.3, 3.0, 1.4), -6),
        ("ServiceWall_North_A", (1260, 1960, 40), (3.0, 0.3, 1.3), 4),
    ]
    for label, location, scale, yaw in walls:
        static_mesh_actor(
            label, cube, location, scale, {"*": materials["stone"]},
            (0, yaw, 0))

    # Industrial barrels clustered in two groups for visual interest.
    barrels = [
        (-230, 270, 10), (-175, 305, 10), (-245, 335, 10),
        (1870, 520, 10), (1925, 550, 10), (1900, 610, 10),
    ]
    for index, location in enumerate(barrels, 1):
        static_mesh_actor(
            f"IndustrialBarrel_{index:02}", cylinder, location,
            (0.34, 0.34, 0.7), {"*": materials["industrial"]})

    # Trees scattered around the arena boundary.
    tree_materials = {
        "White": materials["wood"], "Black": materials["wood"],
        "Green": materials["foliage"], "DarkGreen": materials["foliage"],
    }
    trees = [
        ((1900, 180, -13), 1.65, -20), ((2110, 850, -13), 1.9, 24),
        ((1940, 1780, -13), 1.7, 8), ((1180, 2100, -13), 1.85, -32),
        ((340, 2040, -13), 1.6, 18), ((-360, 1740, -13), 1.75, -8),
    ]
    for index, (location, uniform_scale, yaw) in enumerate(trees, 1):
        static_mesh_actor(
            f"BirchTree_{index:02}", "/Game/Art/Meshes/SM_BirchTree",
            location, (uniform_scale,) * 3, tree_materials, (0, yaw, 0))

    # Stumps and rocks for variety.
    stump_materials = {
        "Wood": materials["wood"], "LightWood": materials["wood"],
        "Green": materials["foliage"],
    }
    for index, (location, scale, yaw) in enumerate([
            ((-430, 1250, -24), 1.5, 12), ((1780, 2020, -24), 1.25, -18)], 1):
        static_mesh_actor(
            f"TreeStump_{index:02}", "/Game/Art/Meshes/SM_TreeStump",
            location, (scale,) * 3, stump_materials, (0, yaw, 0))

    for index, (location, scale) in enumerate([
            ((-460, 420, 0), 2.2), ((2070, 1420, 0), 2.8),
            ((620, 1990, 0), 1.8), ((1820, -260, 0), 2.4)], 1):
        static_mesh_actor(
            f"Rock_{index:02}", "/Game/Art/Meshes/SM_Rock", location,
            (scale, scale, scale * 0.8), {"*": materials["stone"]},
            (0, index * 37, 0))


def _spawn_gameplay_actors(blueprints):
    """Spawn the arena renderer, two tank actors, isometric camera, and player start."""
    renderer = spawn(generated_class(blueprints["renderer"]),
                     "BP_AWArenaRenderer", (0, 0, 0))
    tank_class = generated_class(blueprints["tank"])

    # Tank P1 — cyan accent, bottom-left spawn matching sim (1,1).
    tank_one = spawn(tank_class, "Tank_PlayerOne", (150, 150, 50),
                     (0, 90, 0))
    tank_one.set_editor_property("RobotIndex", 0)
    tank_one.set_editor_property(
        "TankAsset", require_asset("/Game/Art/Meshes/SM_Tank_PlayerOne"))
    tank_one.set_editor_property(
        "CannonAsset", require_asset("/Game/Art/Meshes/SkeletalMeshes/SM_CannonOne"))
    tank_one.set_editor_property(
        "MeshTransform",
        unreal.Transform(location=[6.75, 0.4, -50.0],
                         scale=[0.055, 0.055, 0.055]))
    tank_one.set_editor_property(
        "CannonTransform",
        unreal.Transform(location=[6.75, 0.4, -50.0],
                         scale=[0.055, 0.055, 0.055]))
    tank_one.set_editor_property(
        "PlayerColor", unreal.LinearColor(0.0, 0.78, 0.9, 1.0))

    # Tank P2 — coral accent, top-right spawn matching sim (w-2, h-2).
    tank_two = spawn(tank_class, "Tank_PlayerTwo", (1450, 1450, 50),
                     (0, -90, 0))
    tank_two.set_editor_property("RobotIndex", 1)
    tank_two.set_editor_property(
        "TankAsset", require_asset("/Game/Art/Meshes/SM_Tank_PlayerTwo"))
    tank_two.set_editor_property(
        "CannonAsset", require_asset("/Game/Art/Meshes/SkeletalMeshes/SM_CannonTwo"))
    tank_two.set_editor_property(
        "MeshTransform",
        unreal.Transform(location=[8.1, 0.4, -50.0],
                         scale=[0.055, 0.055, 0.055]))
    tank_two.set_editor_property(
        "CannonTransform",
        unreal.Transform(location=[8.1, 0.4, -50.0],
                         scale=[0.055, 0.055, 0.055]))
    tank_two.set_editor_property(
        "PlayerColor", unreal.LinearColor(0.96, 0.27, 0.22, 1.0))

    # Wire tank references into the renderer for snapshot-driven animation.
    renderer.set_editor_property("PlayerOneTank", tank_one)
    renderer.set_editor_property("PlayerTwoTank", tank_two)

    # Camera positioned for the standard isometric 45-degree view.
    spawn(
        generated_class(blueprints["camera"]), "BP_AWIsometricCamera",
        (-824, -824, 1856), (-38.94, 45, 0))
    spawn(unreal.PlayerStart, "PlayerStart", (800, 800, 100))


def _setup_lighting():
    """Create the sun, sky light, atmosphere, and sky sphere for the arena."""
    # Warm directional sun as the primary light source.
    sun = spawn(
        unreal.DirectionalLight, "Sun", (800, 800, 1200), (-48, -35, 0))
    sun_component = sun.get_editor_property("directional_light_component")
    sun_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun_component.set_intensity(3.5)
    sun_component.set_light_color(
        unreal.LinearColor(1.0, 0.88, 0.7, 1.0), True)
    set_optional(sun_component, "atmosphere_sun_light", True)

    # Ambient sky light with real-time capture for accurate bounce.
    skylight = spawn(unreal.SkyLight, "SkyLight", (800, 800, 800))
    skylight_component = skylight.get_editor_property("light_component")
    skylight_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    skylight_component.set_intensity(1.25)
    set_optional(skylight_component, "real_time_capture", True)

    # Atmosphere and sky sphere for the backdrop.
    spawn(unreal.SkyAtmosphere, "SkyAtmosphere", (0, 0, 0))
    sky_class = unreal.load_class(
        None, "/Engine/EngineSky/BP_Sky_Sphere.BP_Sky_Sphere_C")
    if sky_class:
        spawn(sky_class, "SkySphere", (800, 800, 0))
    else:
        static_mesh_actor(
            "SkySphere", "/Engine/EngineSky/SM_SkySphere", (800, 800, 0),
            (400, 400, 400), {})


def _setup_atmosphere_and_post_process(blueprints):
    """Configure fog, post-processing, world game mode, and save the level."""
    # Volumetric height fog for depth cues.
    fog = spawn(unreal.ExponentialHeightFog, "HeightFog", (800, 800, -25))
    fog_component = fog.get_editor_property("component")
    set_optional(fog_component, "fog_density", 0.008)
    set_optional(fog_component, "fog_height_falloff", 0.22)
    set_optional(fog_component, "enable_volumetric_fog", True)

    # Unbound post-process volume: subtle bloom + vignette, neutral exposure.
    post_process = spawn(unreal.PostProcessVolume,
                         "PostProcess", (800, 800, 0))
    post_process.set_editor_property("unbound", True)
    settings = post_process.get_editor_property("settings")
    set_optional(settings, "override_auto_exposure_bias", True)
    set_optional(settings, "auto_exposure_bias", 0.0)
    set_optional(settings, "override_bloom_intensity", True)
    set_optional(settings, "bloom_intensity", 0.25)
    set_optional(settings, "override_vignette_intensity", True)
    set_optional(settings, "vignette_intensity", 0.18)
    post_process.set_editor_property("settings", settings)

    # Assign the arena game mode as the level default.
    editor_world = unreal.get_editor_subsystem(
        unreal.UnrealEditorSubsystem).get_editor_world()
    if not editor_world:
        editor_world = unreal.EditorLevelLibrary.get_editor_world()
    editor_world.get_world_settings().set_editor_property(
        "default_game_mode", generated_class(blueprints["game_mode"]))

    if not level_editor.save_current_level():
        raise RuntimeError(f"Failed to save {MAP_PATH}")


def build_level(blueprints):
    """Orchestrate full arena level construction from an empty map."""
    _create_or_load_level()
    materials = _load_materials()
    _build_floor_geometry(materials)
    _build_environment_dressing(materials)
    _spawn_gameplay_actors(blueprints)
    _setup_lighting()
    _setup_atmosphere_and_post_process(blueprints)


blueprints = configure_blueprints()
build_level(blueprints)
unreal.EditorAssetLibrary.save_directory(
    BLUEPRINT_DIR, only_if_is_dirty=False, recursive=True)
unreal.EditorAssetLibrary.save_directory(
    UI_DIR, only_if_is_dirty=False, recursive=True)
unreal.log("AUTOMATA_ARENA_COMPLETE")
