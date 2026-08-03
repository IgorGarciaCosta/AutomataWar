import unreal


MAP_PATH = "/Game/Maps/L_AutomataArena"
TANK_BLUEPRINT_PATH = "/Game/Blueprints/BP_TankActor"
TANK_NATIVE_CLASS_PATH = "/Script/AutomataWar.AWTankActor"
RENDERER_CLASS_PATH = "/Script/AutomataWar.AWArenaRenderer"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

tank_blueprint = unreal.load_asset(TANK_BLUEPRINT_PATH)
if not tank_blueprint:
    tank_factory = unreal.BlueprintFactory()
    tank_factory.set_editor_property(
        "parent_class", unreal.load_class(None, TANK_NATIVE_CLASS_PATH))
    tank_blueprint = asset_tools.create_asset(
        "BP_TankActor", "/Game/Blueprints", None, tank_factory)
if not tank_blueprint:
    raise RuntimeError("Failed to create BP_TankActor")
unreal.BlueprintEditorLibrary.compile_blueprint(tank_blueprint)
unreal.EditorAssetLibrary.save_loaded_asset(
    tank_blueprint, only_if_is_dirty=False)

if not level_editor.load_level(MAP_PATH):
    raise RuntimeError(f"Failed to load {MAP_PATH}")

tank_class = tank_blueprint.generated_class()
renderer_class = unreal.load_class(None, RENDERER_CLASS_PATH)
if not tank_class or not renderer_class:
    raise RuntimeError(
        "Tank Blueprint or renderer native class is unavailable")

renderer = None
for actor in actor_editor.get_all_level_actors():
    if actor.get_actor_label() in ["Tank_PlayerOne", "Tank_PlayerTwo"]:
        actor_editor.destroy_actor(actor)
    elif actor.get_actor_label() == "BP_AWArenaRenderer":
        renderer = actor

if not renderer:
    raise RuntimeError("BP_AWArenaRenderer is not present in the arena map")


def spawn_tank(label, robot_index, mesh_path, location, yaw,
               mesh_location, color):
    tank = actor_editor.spawn_actor_from_class(
        tank_class, unreal.Vector(*location), unreal.Rotator(0.0, yaw, 0.0))
    if not tank:
        raise RuntimeError(f"Failed to spawn {label}")
    tank.set_actor_label(label)
    tank.set_editor_property("RobotIndex", robot_index)
    tank.set_editor_property("TankAsset", unreal.load_asset(mesh_path))
    tank.set_editor_property(
        "MeshTransform",
        unreal.Transform(location=mesh_location, scale=[0.055, 0.055, 0.055]))
    tank.set_editor_property("PlayerColor", unreal.LinearColor(*color))
    return tank


tank_one = spawn_tank(
    "Tank_PlayerOne", 0, "/Game/Art/Meshes/SM_Tank_PlayerOne",
    (150.0, 150.0, 50.0), 90.0, [6.75, 0.4, -50.0],
    [0.0, 0.78, 0.9, 1.0])
tank_two = spawn_tank(
    "Tank_PlayerTwo", 1, "/Game/Art/Meshes/SM_Tank_PlayerTwo",
    (1450.0, 1450.0, 50.0), -90.0, [8.1, 0.4, -50.0],
    [0.96, 0.27, 0.22, 1.0])

renderer.set_editor_property("PlayerOneTank", tank_one)
renderer.set_editor_property("PlayerTwoTank", tank_two)

if not level_editor.save_current_level():
    raise RuntimeError(f"Failed to save {MAP_PATH}")

unreal.log("AUTOMATA_TANK_MIGRATION_COMPLETE")
