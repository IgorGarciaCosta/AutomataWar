import os
import shutil
import tempfile
import urllib.request
import zipfile
import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(
    unreal.Paths.project_dir())
CONTENT_DIR = unreal.Paths.convert_relative_path_to_full(
    unreal.Paths.project_content_dir())
STAGE_DIR = os.path.join(tempfile.gettempdir(), "AutomataWarAssetGeneration")

FONT_FILES = {
    "F_AWMono.ttf": "https://raw.githubusercontent.com/google/fonts/main/ofl/robotomono/RobotoMono%5Bwght%5D.ttf",
    "F_AWDisplay.ttf": "https://raw.githubusercontent.com/google/fonts/main/ofl/rajdhani/Rajdhani-SemiBold.ttf",
}

PACKS = {
    "scifi": "https://kenney.nl/media/pages/assets/sci-fi-sounds/6b296f9ecf-1677589334/kenney_sci-fi-sounds.zip",
    "ui": "https://kenney.nl/media/pages/assets/ui-audio/490d233f68-1677590494/kenney_ui-audio.zip",
}

MODEL_FILES = {
    "SM_Tank_PlayerOne": "https://drive.usercontent.google.com/download?id=13lbAEkwen5bXY2fSiyo4rVt5q_gxy0Fm&export=download&confirm=t",
    "SM_Tank_PlayerTwo": "https://drive.usercontent.google.com/download?id=1a57hqb1yEdLCbeQiQQjf2mobANJNc5dc&export=download&confirm=t",
    "SM_BirchTree": "https://drive.usercontent.google.com/download?id=1FmqkvK2dFjcG_Cx4umWDZ5bR7uS2fDuX&export=download&confirm=t",
    "SM_Rock": "https://drive.usercontent.google.com/download?id=1dQ8kOkuHLvtO7TNxUeyfdPdxje3gn_Aa&export=download&confirm=t",
    "SM_TreeStump": "https://drive.usercontent.google.com/download?id=1QOtVG-SdMuSkM5OiLyfIjZWvzfvlBY5G&export=download&confirm=t",
}

AUDIO_FILES = {
    "S_Fire": ("scifi", "Audio/laserSmall_001.ogg"),
    "S_Impact": ("scifi", "Audio/impactMetal_002.ogg"),
    "S_Shield": ("scifi", "Audio/forceField_003.ogg"),
    "S_Move": ("scifi", "Audio/thrusterFire_000.ogg"),
    "S_Destroy": ("scifi", "Audio/explosionCrunch_004.ogg"),
    "S_MatchStart": ("scifi", "Audio/doorOpen_002.ogg"),
    "S_MatchEnd": ("scifi", "Audio/doorClose_002.ogg"),
    "S_UIConfirm": ("ui", "Audio/click1.ogg"),
    "S_UINavigate": ("ui", "Audio/rollover2.ogg"),
    "S_UIError": ("ui", "Audio/switch26.ogg"),
}

NIAGARA_TEMPLATES = {
    "NS_MuzzleFlash": "/Niagara/DefaultAssets/Templates/Systems/DirectionalBurst.DirectionalBurst",
    "NS_ProjectileTrail": "/Niagara/DefaultAssets/Templates/Systems/AttributeReaderTrails.AttributeReaderTrails",
    "NS_Impact": "/Niagara/DefaultAssets/Templates/Systems/RadialBurst.RadialBurst",
    "NS_Shield": "/Niagara/DefaultAssets/Templates/Systems/RadialBurst.RadialBurst",
    "NS_Destruction": "/Niagara/DefaultAssets/Templates/Systems/SimpleExplosion.SimpleExplosion",
}


def download(url, destination):
    os.makedirs(os.path.dirname(destination), exist_ok=True)
    if not os.path.isfile(destination):
        urllib.request.urlretrieve(url, destination)


def prepare_sources():
    shutil.rmtree(STAGE_DIR, ignore_errors=True)
    os.makedirs(STAGE_DIR, exist_ok=True)
    font_dir = os.path.join(CONTENT_DIR, "UI", "Fonts")
    os.makedirs(font_dir, exist_ok=True)
    for filename, url in FONT_FILES.items():
        download(url, os.path.join(font_dir, filename))

    pack_dirs = {}
    for key, url in PACKS.items():
        archive = os.path.join(STAGE_DIR, key + ".zip")
        destination = os.path.join(STAGE_DIR, key)
        download(url, archive)
        with zipfile.ZipFile(archive) as package:
            package.extractall(destination)
        pack_dirs[key] = destination

    model_files = {}
    for asset_name, url in MODEL_FILES.items():
        filename = os.path.join(STAGE_DIR, "models", asset_name + ".fbx")
        download(url, filename)
        model_files[asset_name] = filename
    return pack_dirs, model_files


def import_audio(pack_dirs):
    tasks = []
    for asset_name, (pack, relative_path) in AUDIO_FILES.items():
        task = unreal.AssetImportTask()
        task.filename = os.path.join(
            pack_dirs[pack], *relative_path.split("/"))
        task.destination_path = "/Game/Audio/SFX"
        task.destination_name = asset_name
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    for task in tasks:
        if not task.imported_object_paths:
            raise RuntimeError(f"Audio import failed: {task.destination_name}")


def import_models(model_files):
    tasks = []
    for asset_name, filename in model_files.items():
        options = unreal.FbxImportUI()
        options.set_editor_property(
            "mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
        options.set_editor_property(
            "automated_import_should_detect_type", False)
        options.set_editor_property("import_as_skeletal", False)
        options.set_editor_property("import_animations", False)
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)
        options.set_editor_property("override_full_name", True)
        static_options = options.get_editor_property("static_mesh_import_data")
        static_options.set_editor_property("combine_meshes", True)
        static_options.set_editor_property("auto_generate_collision", False)
        static_options.set_editor_property("generate_lightmap_u_vs", True)
        static_options.set_editor_property("build_nanite", False)

        task = unreal.AssetImportTask()
        task.filename = filename
        task.destination_path = "/Game/Art/Meshes"
        task.destination_name = asset_name
        task.automated = True
        task.replace_existing = True
        task.save = True
        task.options = options
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    for task in tasks:
        path = f"/Game/Art/Meshes/{task.destination_name}"
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            raise RuntimeError(f"Model import failed: {task.destination_name}")


def recreate_material(name):
    path = f"/Game/Art/Materials/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, "/Game/Art/Materials", unreal.Material, unreal.MaterialFactoryNew()
    )
    if material is None:
        raise RuntimeError(f"Material creation failed: {name}")
    return material


def vector_parameter(material, name, value, x, y):
    expression = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, x, y
    )
    expression.set_editor_property("parameter_name", name)
    expression.set_editor_property("default_value", value)
    return expression


def scalar_parameter(material, name, value, x, y):
    expression = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, x, y
    )
    expression.set_editor_property("parameter_name", name)
    expression.set_editor_property("default_value", value)
    return expression


def create_surface_material(name, color, metallic, roughness):
    material = recreate_material(name)
    base = vector_parameter(material, "BaseColor", color, -350, -80)
    metal = scalar_parameter(material, "Metallic", metallic, -350, 80)
    rough = scalar_parameter(material, "Roughness", roughness, -350, 160)
    unreal.MaterialEditingLibrary.connect_material_property(
        base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(
        metal, "", unreal.MaterialProperty.MP_METALLIC)
    unreal.MaterialEditingLibrary.connect_material_property(
        rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(material)
    return material


def create_materials():
    arena = recreate_material("M_ArenaCell")
    vertex_color = unreal.MaterialEditingLibrary.create_material_expression(
        arena, unreal.MaterialExpressionVertexColor, -300, 0
    )
    roughness = scalar_parameter(arena, "Roughness", 0.68, -300, 180)
    unreal.MaterialEditingLibrary.connect_material_property(
        vertex_color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(arena)

    robot = recreate_material("M_Robot")
    base = vector_parameter(robot, "BaseColor", unreal.LinearColor(
        0.04, 0.35, 0.42, 1.0), -400, -100)
    emissive = vector_parameter(
        robot, "EmissiveColor", unreal.LinearColor(0.0, 0.04, 0.05, 1.0), -400, 40)
    metallic = scalar_parameter(robot, "Metallic", 0.72, -400, 180)
    rough = scalar_parameter(robot, "Roughness", 0.28, -400, 260)
    unreal.MaterialEditingLibrary.connect_material_property(
        base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(
        emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(
        metallic, "", unreal.MaterialProperty.MP_METALLIC)
    unreal.MaterialEditingLibrary.connect_material_property(
        rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(robot)

    cover = recreate_material("M_Cover")
    base = vector_parameter(cover, "BaseColor", unreal.LinearColor(
        0.12, 0.15, 0.18, 1.0), -400, -80)
    metallic = scalar_parameter(cover, "Metallic", 0.58, -400, 80)
    rough = scalar_parameter(cover, "Roughness", 0.42, -400, 160)
    unreal.MaterialEditingLibrary.connect_material_property(
        base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(
        metallic, "", unreal.MaterialProperty.MP_METALLIC)
    unreal.MaterialEditingLibrary.connect_material_property(
        rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(cover)

    effect = recreate_material("M_Effect")
    effect.set_editor_property(
        "blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    effect.set_editor_property(
        "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    effect.set_editor_property("two_sided", True)
    emissive = vector_parameter(
        effect, "EmissiveColor", unreal.LinearColor(0.0, 2.5, 4.0, 1.0), -350, -40)
    opacity = scalar_parameter(effect, "Opacity", 0.45, -350, 100)
    unreal.MaterialEditingLibrary.connect_material_property(
        emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(
        opacity, "", unreal.MaterialProperty.MP_OPACITY)
    unreal.MaterialEditingLibrary.recompile_material(effect)

    ground = create_surface_material("M_Ground", unreal.LinearColor(
        0.055, 0.075, 0.07, 1.0), 0.08, 0.82)
    foliage = create_surface_material("M_Foliage", unreal.LinearColor(
        0.08, 0.22, 0.12, 1.0), 0.0, 0.88)
    wood = create_surface_material("M_Wood", unreal.LinearColor(
        0.16, 0.08, 0.035, 1.0), 0.0, 0.8)
    stone = create_surface_material("M_Stone", unreal.LinearColor(
        0.16, 0.18, 0.19, 1.0), 0.0, 0.9)
    industrial = create_surface_material("M_Industrial", unreal.LinearColor(
        0.32, 0.18, 0.035, 1.0), 0.45, 0.5)

    for material in (arena, robot, cover, effect, ground, foliage, wood, stone, industrial):
        unreal.EditorAssetLibrary.save_loaded_asset(material)


def create_niagara_systems():
    for name, source in NIAGARA_TEMPLATES.items():
        destination = f"/Game/Art/VFX/{name}"
        if unreal.EditorAssetLibrary.does_asset_exist(destination):
            unreal.EditorAssetLibrary.delete_asset(destination)
        if not unreal.EditorAssetLibrary.duplicate_asset(source, destination):
            raise RuntimeError(f"Niagara duplication failed: {name}")
        unreal.EditorAssetLibrary.save_asset(destination)


def main():
    pack_dirs, model_files = prepare_sources()
    import_audio(pack_dirs)
    import_models(model_files)
    create_materials()
    create_niagara_systems()
    unreal.EditorAssetLibrary.save_directory(
        "/Game/Art", only_if_is_dirty=False, recursive=True)
    unreal.EditorAssetLibrary.save_directory(
        "/Game/Audio", only_if_is_dirty=False, recursive=True)
    unreal.log("AUTOMATA_ASSETS_COMPLETE")


main()
