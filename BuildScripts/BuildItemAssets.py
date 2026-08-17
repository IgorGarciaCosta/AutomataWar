import os
import tempfile
import urllib.request

import unreal


ITEMS = {
    "/Game/Blueprints/Items/BP_ActionPointItem": {
        "parent": "/Script/AutomataWar.AWAPItem",
        "mesh": "SM_Item_Coin",
        "resource": "c4492ad6-e885-486d-b595-22d66dbe95ee",
        "page": "https://poly.pizza/m/QHZtj94fvh",
        "rotation": unreal.Rotator(pitch=90.0),
    },
    "/Game/Blueprints/Items/BP_ExtraAmmoItem": {
        "parent": "/Script/AutomataWar.AWExtraAmmoItem",
        "mesh": "SM_Item_Bullets",
        "resource": "d77c65f0-1e08-4085-a31d-2fed73d51315",
        "page": "https://poly.pizza/m/bTEYFxKHF9",
        "rotation": unreal.Rotator(),
    },
    "/Game/Blueprints/Items/BP_ShieldItem": {
        "parent": "/Script/AutomataWar.AWShieldItem",
        "mesh": "SM_Item_Shield",
        "resource": "60cc7b8e-0589-4f4b-a354-f6fef73a44bd",
        "page": "https://poly.pizza/m/srN1KGAO7f",
        "rotation": unreal.Rotator(),
    },
    "/Game/Blueprints/Items/BP_AcceleratorItem": {
        "parent": "/Script/AutomataWar.AWAcceleratorItem",
        "mesh": "SM_Item_Rocket",
        "resource": "244c027c-40f0-45ca-a707-0f8e855c9831",
        "page": "https://poly.pizza/m/9awwTQWYux",
        "rotation": unreal.Rotator(pitch=90.0),
    },
}
MESH_DIRECTORY = "/Game/Art/Meshes/Items"
SOURCE_DIRECTORY = os.path.join(tempfile.gettempdir(), "AutomataWarItemMeshes")
TARGET_MESH_SIZE = 68.0

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def download_mesh(mesh_name, resource_id, page_url):
    os.makedirs(SOURCE_DIRECTORY, exist_ok=True)
    filename = os.path.join(SOURCE_DIRECTORY, mesh_name + ".glb")
    request = urllib.request.Request(
        f"https://static.poly.pizza/{resource_id}.glb",
        headers={"User-Agent": "Mozilla/5.0", "Referer": page_url},
    )
    with urllib.request.urlopen(request) as response, open(filename, "wb") as output:
        output.write(response.read())
    with open(filename, "rb") as source:
        if source.read(4) != b"glTF":
            raise RuntimeError(f"Invalid GLB download: {mesh_name}")
    return filename


def import_mesh(mesh_name, filename):
    destination = f"{MESH_DIRECTORY}/{mesh_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(destination):
        unreal.EditorAssetLibrary.delete_asset(destination)
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = MESH_DIRECTORY
    task.destination_name = mesh_name
    task.automated = True
    task.replace_existing = True
    task.save = True
    asset_tools.import_asset_tasks([task])

    meshes = []
    for path in task.imported_object_paths:
        asset = unreal.load_asset(path)
        if isinstance(asset, unreal.StaticMesh):
            meshes.append(asset)
    if len(meshes) != 1:
        raise RuntimeError(
            f"Expected one static mesh for {mesh_name}, imported {task.imported_object_paths}")
    mesh = meshes[0]
    current_path = mesh.get_path_name().split(".", 1)[0]
    if current_path != destination:
        if not unreal.EditorAssetLibrary.rename_asset(current_path, destination):
            raise RuntimeError(f"Failed to rename imported mesh: {mesh_name}")
        mesh = unreal.load_asset(destination)
    return mesh


def get_or_create_blueprint(asset_path, parent_path):
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
    return blueprint


def assign_mesh(blueprint, mesh, rotation):
    item_cdo = unreal.get_default_object(blueprint.generated_class())
    item_mesh = next((
        component
        for component in item_cdo.get_components_by_class(
            unreal.StaticMeshComponent)
        if component.get_name() == "ItemMesh"
    ), None)
    if not item_mesh:
        raise RuntimeError(f"{blueprint.get_path_name()} has no ItemMesh")

    bounds = mesh.get_bounds()
    max_dimension = 2.0 * max(
        bounds.box_extent.x, bounds.box_extent.y, bounds.box_extent.z)
    if max_dimension <= 0.0:
        raise RuntimeError(f"{mesh.get_path_name()} has invalid bounds")
    scale = TARGET_MESH_SIZE / max_dimension
    item_mesh.modify()
    item_mesh.set_editor_property("static_mesh", mesh)
    item_mesh.set_editor_property("relative_rotation", rotation)
    item_mesh.set_editor_property(
        "relative_scale3d", unreal.Vector(scale, scale, scale))
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(
        blueprint, only_if_is_dirty=False)


def main():
    imported_meshes = {}
    for item in ITEMS.values():
        mesh_name = item["mesh"]
        imported_meshes[mesh_name] = import_mesh(
            mesh_name,
            download_mesh(mesh_name, item["resource"], item["page"]),
        )
    for asset_path, item in ITEMS.items():
        blueprint = get_or_create_blueprint(asset_path, item["parent"])
        assign_mesh(blueprint, imported_meshes[item["mesh"]], item["rotation"])
        native_parent = unreal.EditorAssetLibrary.find_asset_data(
            asset_path).get_tag_value("NativeParentClass")
        if item["parent"] not in str(native_parent):
            raise RuntimeError(f"{asset_path} has the wrong native parent")
    unreal.log(f"AUTOMATA_ITEM_ASSETS_COMPLETE count={len(ITEMS)}")


if __name__ == "__main__":
    main()
