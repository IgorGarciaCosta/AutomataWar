import unreal


SYSTEM_SOURCES = {
    "NS_MuzzleFlash": "/Game/MuzzleFlash/MuzzleFlash/Niagara/NS_MuzzleFlash",
    "NS_Impact": "/Game/Vefects/Easy_Impact_Frames/VFX/Frames/Particles/NS_Impact_Frame_Advanced_01_Always",
}
SOURCE_ROOTS = {
    SYSTEM_SOURCES["NS_MuzzleFlash"]: "/Game/MuzzleFlash",
    SYSTEM_SOURCES["NS_Impact"]: "/Game/Vefects/Easy_Impact_Frames",
}
SOURCE_METADATA_TAG = "AutomataWar.SourcePackage"
OBSOLETE_DIRECTORIES = [
    "/Game/Art/VFX/Materials",
    "/Game/Art/VFX/Textures",
    "/Game/FreeNiagaraPack",
]

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()


def dependency_options():
    options = unreal.AssetRegistryDependencyOptions()
    options.set_editor_property("include_soft_package_references", True)
    options.set_editor_property("include_hard_package_references", True)
    options.set_editor_property("include_searchable_names", False)
    options.set_editor_property("include_soft_management_references", False)
    options.set_editor_property("include_hard_management_references", False)
    return options


def trim_package(source_path, root_path):
    keep = {source_path}
    pending = [source_path]
    options = dependency_options()
    while pending:
        package = pending.pop()
        for dependency in asset_registry.get_dependencies(package, options):
            dependency = str(dependency)
            if dependency.startswith(root_path) and dependency not in keep:
                keep.add(dependency)
                pending.append(dependency)

    removed = 0
    for asset_data in asset_registry.get_assets_by_path(root_path, recursive=True):
        package_name = str(asset_data.package_name)
        if package_name not in keep:
            if not unreal.EditorAssetLibrary.delete_asset(package_name):
                raise RuntimeError(f"Failed to trim unused package asset: {package_name}")
            removed += 1
    unreal.log(
        f"AUTOMATA_VFX_PACKAGE_TRIMMED root={root_path} keep={len(keep)} removed={removed}")


def duplicate_package_system(asset_name, source_path):
    destination = f"/Game/Art/VFX/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(destination):
        unreal.EditorAssetLibrary.delete_asset(destination)

    source = unreal.load_asset(source_path)
    if not source or not isinstance(source, unreal.NiagaraSystem):
        raise RuntimeError(f"Missing packaged Niagara system: {source_path}")
    system = asset_tools.duplicate_asset(asset_name, "/Game/Art/VFX", source)
    if not system:
        raise RuntimeError(
            f"Failed to duplicate packaged Niagara system: {asset_name}")
    unreal.EditorAssetLibrary.set_metadata_tag(
        system, SOURCE_METADATA_TAG, source_path)
    unreal.EditorAssetLibrary.save_loaded_asset(
        system, only_if_is_dirty=False)


def main():
    for directory in OBSOLETE_DIRECTORIES:
        if unreal.EditorAssetLibrary.does_directory_exist(directory):
            unreal.EditorAssetLibrary.delete_directory(directory)
    for asset_name, source_path in SYSTEM_SOURCES.items():
        duplicate_package_system(asset_name, source_path)
    for source_path, root_path in SOURCE_ROOTS.items():
        trim_package(source_path, root_path)
    unreal.log(f"AUTOMATA_VFX_ASSETS_COMPLETE systems={len(SYSTEM_SOURCES)}")


if __name__ == "__main__":
    main()
