import os
import shutil
import tempfile
import urllib.request
import zipfile

import unreal


PACK_URL = "https://kenney.nl/media/pages/assets/ui-audio/490d233f68-1677590494/kenney_ui-audio.zip"
SOUNDS = {
    "S_UINavigate": "Audio/click1.ogg",
    "S_UICommand": "Audio/click2.ogg",
    "S_UIConfirm": "Audio/click3.ogg",
    "S_UIDanger": "Audio/click4.ogg",
    "S_UITransport": "Audio/click5.ogg",
    "S_UIHover": "Audio/rollover2.ogg",
    "S_UIError": "Audio/switch26.ogg",
}


stage_dir = os.path.join(tempfile.gettempdir(), "AutomataWarUIAudio")
archive = os.path.join(stage_dir, "kenney_ui-audio.zip")
sources = os.path.join(stage_dir, "sources")
shutil.rmtree(stage_dir, ignore_errors=True)
os.makedirs(stage_dir, exist_ok=True)
urllib.request.urlretrieve(PACK_URL, archive)
with zipfile.ZipFile(archive) as package:
    package.extractall(sources)

movement_sound = "/Game/Audio/SFX/S_Move"
if unreal.EditorAssetLibrary.does_asset_exist(movement_sound):
    unreal.EditorAssetLibrary.delete_asset(movement_sound)

tasks = []
for asset_name, relative_path in SOUNDS.items():
    task = unreal.AssetImportTask()
    task.filename = os.path.join(sources, *relative_path.split("/"))
    task.destination_path = "/Game/Audio/SFX"
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.save = True
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
failed = [task.destination_name for task in tasks
          if not task.imported_object_paths]
if failed:
    raise RuntimeError(f"UI audio import failed: {failed}")

unreal.log("AUTOMATA_UI_AUDIO_COMPLETE")