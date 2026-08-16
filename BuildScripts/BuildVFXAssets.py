import os
import tempfile
import urllib.request
import zipfile

import unreal


PACK_URL = (
    "https://kenney.nl/media/pages/assets/particle-pack/"
    "f8fe0f8cb8-1677578741/kenney_particle-pack.zip"
)
PACK_FILE = os.path.join(tempfile.gettempdir(), "kenney_particle-pack.zip")
SOURCE_DIR = os.path.join(tempfile.gettempdir(), "AutomataWarVFX")
TEXTURES = {
    "T_VFX_Muzzle_Kenney": "PNG (Transparent)/muzzle_01.png",
    "T_VFX_Impact_Kenney": "PNG (Transparent)/scorch_01.png",
}
SYSTEMS = {
    "NS_MuzzleFlash": (
        "/Niagara/DefaultAssets/Templates/Systems/DirectionalBurst.DirectionalBurst",
        "M_VFX_Muzzle_Kenney",
    ),
    "NS_Impact": (
        "/Niagara/DefaultAssets/Templates/Systems/RadialBurst.RadialBurst",
        "M_VFX_Impact_Kenney",
    ),
}

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
material_library = unreal.MaterialEditingLibrary
bridge = unreal.AWWidgetBlueprintLibrary


def prepare_sources():
    os.makedirs(SOURCE_DIR, exist_ok=True)
    if not os.path.isfile(PACK_FILE):
        urllib.request.urlretrieve(PACK_URL, PACK_FILE)

    source_files = {}
    with zipfile.ZipFile(PACK_FILE) as package:
        for asset_name, archive_path in TEXTURES.items():
            destination = os.path.join(SOURCE_DIR, asset_name + ".png")
            with package.open(archive_path) as source, open(destination, "wb") as output:
                output.write(source.read())
            source_files[asset_name] = destination
    return source_files


def import_textures(source_files):
    tasks = []
    for asset_name, filename in source_files.items():
        task = unreal.AssetImportTask()
        task.filename = filename
        task.destination_path = "/Game/Art/VFX/Textures"
        task.destination_name = asset_name
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)
    asset_tools.import_asset_tasks(tasks)

    textures = {}
    for asset_name in source_files:
        texture = unreal.load_asset(f"/Game/Art/VFX/Textures/{asset_name}")
        if not texture:
            raise RuntimeError(f"Failed to import VFX texture: {asset_name}")
        textures[asset_name] = texture
    return textures


def expression(material, expression_class, x, y):
    return material_library.create_material_expression(
        material, expression_class, x, y)


def create_sprite_material(name, texture, tint):
    asset_path = f"/Game/Art/VFX/Materials/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.EditorAssetLibrary.delete_asset(asset_path)
    material = asset_tools.create_asset(
        name,
        "/Game/Art/VFX/Materials",
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if not material:
        raise RuntimeError(f"Failed to create VFX material: {name}")

    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property(
        "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("used_with_niagara_sprites", True)

    texture_sample = expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -760, -80)
    texture_sample.set_editor_property("parameter_name", "SpriteTexture")
    texture_sample.set_editor_property("texture", texture)

    tint_parameter = expression(
        material, unreal.MaterialExpressionVectorParameter, -760, 100)
    tint_parameter.set_editor_property("parameter_name", "Tint")
    tint_parameter.set_editor_property("default_value", tint)
    particle_color = expression(
        material, unreal.MaterialExpressionParticleColor, -760, 260)

    tinted_sprite = expression(
        material, unreal.MaterialExpressionMultiply, -430, -40)
    material_library.connect_material_expressions(
        texture_sample, "RGB", tinted_sprite, "A")
    material_library.connect_material_expressions(
        tint_parameter, "", tinted_sprite, "B")
    final_color = expression(
        material, unreal.MaterialExpressionMultiply, -190, -40)
    material_library.connect_material_expressions(
        tinted_sprite, "", final_color, "A")
    material_library.connect_material_expressions(
        particle_color, "RGB", final_color, "B")

    final_opacity = expression(
        material, unreal.MaterialExpressionMultiply, -190, 180)
    material_library.connect_material_expressions(
        texture_sample, "A", final_opacity, "A")
    material_library.connect_material_expressions(
        particle_color, "A", final_opacity, "B")
    material_library.connect_material_property(
        final_color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    material_library.connect_material_property(
        final_opacity, "", unreal.MaterialProperty.MP_OPACITY)
    material_library.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(
        material, only_if_is_dirty=False)
    return material


def create_system(name, source_path, material):
    destination = f"/Game/Art/VFX/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(destination):
        unreal.EditorAssetLibrary.delete_asset(destination)
    source_system = unreal.load_asset(source_path)
    if not source_system:
        raise RuntimeError(f"Missing Niagara template: {source_path}")
    system = asset_tools.duplicate_asset(
        name, "/Game/Art/VFX", source_system)
    if not system:
        raise RuntimeError(f"Failed to duplicate Niagara system: {name}")

    changed_renderers = bridge.set_niagara_sprite_material(system, material)
    if changed_renderers == 0:
        raise RuntimeError(f"{name} contains no editable sprite renderers")
    unreal.EditorAssetLibrary.save_loaded_asset(
        system, only_if_is_dirty=False)
    return changed_renderers


def main():
    textures = import_textures(prepare_sources())
    materials = {
        "M_VFX_Muzzle_Kenney": create_sprite_material(
            "M_VFX_Muzzle_Kenney",
            textures["T_VFX_Muzzle_Kenney"],
            unreal.LinearColor(5.0, 1.15, 0.08, 1.0),
        ),
        "M_VFX_Impact_Kenney": create_sprite_material(
            "M_VFX_Impact_Kenney",
            textures["T_VFX_Impact_Kenney"],
            unreal.LinearColor(0.2, 2.8, 4.5, 1.0),
        ),
    }

    changed_renderers = 0
    for name, (source_path, material_name) in SYSTEMS.items():
        changed_renderers += create_system(
            name, source_path, materials[material_name])
    unreal.log(
        f"AUTOMATA_VFX_ASSETS_COMPLETE renderers={changed_renderers}")


if __name__ == "__main__":
    main()