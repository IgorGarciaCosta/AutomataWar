# Third-Party Credits

Original asset-pack acquisition date: **2026-08-01**. Additional UI variants were selected from the same CC0 pack on **2026-08-13**, and stylized VFX sprites were selected on **2026-08-16**.

No asset with unclear, non-commercial, or no-derivatives terms is included. Project-authored C++ code, materials, procedural layouts, screenshots, and mesh assemblies are not third-party assets.

## Fonts

| Project file                       | Asset                                                 | Author / copyright                                                  | Source                                                                              | License                                           |
| ---------------------------------- | ----------------------------------------------------- | ------------------------------------------------------------------- | ----------------------------------------------------------------------------------- | ------------------------------------------------- |
| `Content/UI/Fonts/F_AWMono.ttf`    | Roboto Mono variable regular (`RobotoMono[wght].ttf`) | Christian Robertson; Copyright 2015 The Roboto Mono Project Authors | [Google Fonts repository](https://github.com/google/fonts/tree/main/ofl/robotomono) | [SIL Open Font License 1.1](Licenses/OFL-1.1.txt) |
| `Content/UI/Fonts/F_AWDisplay.ttf` | Rajdhani SemiBold                                     | Indian Type Foundry; Copyright (c) 2014 Indian Type Foundry         | [Google Fonts repository](https://github.com/google/fonts/tree/main/ofl/rajdhani)   | [SIL Open Font License 1.1](Licenses/OFL-1.1.txt) |

## Audio

All audio below is by **Kenney** and is released under [Creative Commons CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/).

| Project asset                            | Original file                   | Pack and source                                                |
| ---------------------------------------- | ------------------------------- | -------------------------------------------------------------- |
| `Content/Audio/SFX/S_Fire.uasset`        | `Audio/laserSmall_001.ogg`      | [Kenney Sci-fi Sounds](https://kenney.nl/assets/sci-fi-sounds) |
| `Content/Audio/SFX/S_Impact.uasset`      | `Audio/impactMetal_002.ogg`     | [Kenney Sci-fi Sounds](https://kenney.nl/assets/sci-fi-sounds) |
| `Content/Audio/SFX/S_Shield.uasset`      | `Audio/forceField_003.ogg`      | [Kenney Sci-fi Sounds](https://kenney.nl/assets/sci-fi-sounds) |
| `Content/Audio/SFX/S_Destroy.uasset`     | `Audio/explosionCrunch_004.ogg` | [Kenney Sci-fi Sounds](https://kenney.nl/assets/sci-fi-sounds) |
| `Content/Audio/SFX/S_MatchStart.uasset`  | `Audio/doorOpen_002.ogg`        | [Kenney Sci-fi Sounds](https://kenney.nl/assets/sci-fi-sounds) |
| `Content/Audio/SFX/S_MatchEnd.uasset`    | `Audio/doorClose_002.ogg`       | [Kenney Sci-fi Sounds](https://kenney.nl/assets/sci-fi-sounds) |
| `Content/Audio/SFX/S_UINavigate.uasset`  | `Audio/click1.ogg`              | [Kenney UI Audio](https://kenney.nl/assets/ui-audio)           |
| `Content/Audio/SFX/S_UICommand.uasset`   | `Audio/click2.ogg`              | [Kenney UI Audio](https://kenney.nl/assets/ui-audio)           |
| `Content/Audio/SFX/S_UIConfirm.uasset`   | `Audio/click3.ogg`              | [Kenney UI Audio](https://kenney.nl/assets/ui-audio)           |
| `Content/Audio/SFX/S_UIDanger.uasset`    | `Audio/click4.ogg`              | [Kenney UI Audio](https://kenney.nl/assets/ui-audio)           |
| `Content/Audio/SFX/S_UITransport.uasset` | `Audio/click5.ogg`              | [Kenney UI Audio](https://kenney.nl/assets/ui-audio)           |
| `Content/Audio/SFX/S_UIHover.uasset`     | `Audio/rollover2.ogg`           | [Kenney UI Audio](https://kenney.nl/assets/ui-audio)           |
| `Content/Audio/SFX/S_UIError.uasset`     | `Audio/switch26.ogg`            | [Kenney UI Audio](https://kenney.nl/assets/ui-audio)           |

## VFX Textures

Both textures below are by **Kenney** and are released under [Creative Commons CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/). Project-authored unlit materials apply them to UE 5.5 Niagara template emitters.

| Project asset                                         | Original file                     | Pack and source                                                |
| ----------------------------------------------------- | --------------------------------- | -------------------------------------------------------------- |
| `Content/Art/VFX/Textures/T_VFX_Muzzle_Kenney.uasset` | `PNG (Transparent)/muzzle_01.png` | [Kenney Particle Pack](https://kenney.nl/assets/particle-pack) |
| `Content/Art/VFX/Textures/T_VFX_Impact_Kenney.uasset` | `PNG (Transparent)/scorch_01.png` | [Kenney Particle Pack](https://kenney.nl/assets/particle-pack) |

## 3D Models

All models below are by **Quaternius** and are released under [Creative Commons CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/). They are imported as lightweight combined static meshes without textures, animation data, collision, or Nanite.

| Project asset                                 | Original file     | Pack and source                                                                     |
| --------------------------------------------- | ----------------- | ----------------------------------------------------------------------------------- |
| `Content/Art/Meshes/SM_Tank_PlayerOne.uasset` | `Tank.fbx`        | [Quaternius Animated Tanks Pack](https://quaternius.com/packs/animatedtanks.html)   |
| `Content/Art/Meshes/SM_Tank_PlayerTwo.uasset` | `Tank2.fbx`       | [Quaternius Animated Tanks Pack](https://quaternius.com/packs/animatedtanks.html)   |
| `Content/Art/Meshes/SM_BirchTree.uasset`      | `BirchTree_3.fbx` | [Quaternius Ultimate Nature Pack](https://quaternius.com/packs/ultimatenature.html) |
| `Content/Art/Meshes/SM_Rock.uasset`           | `Rock_3.fbx`      | [Quaternius Ultimate Nature Pack](https://quaternius.com/packs/ultimatenature.html) |
| `Content/Art/Meshes/SM_TreeStump.uasset`      | `TreeStump.fbx`   | [Quaternius Ultimate Nature Pack](https://quaternius.com/packs/ultimatenature.html) |

## Unreal Engine Content

The following assets are Epic-provided Unreal Engine 5.5 Licensed Technology used under the [Unreal Engine EULA](https://www.unrealengine.com/eula/unreal).

| Project use                                                 | Epic source                                                                              | Derived project assets                                                   |
| ----------------------------------------------------------- | ---------------------------------------------------------------------------------------- | ------------------------------------------------------------------------ |
| Arena foundation, rails, walls, barrels, cover, and effects | `/Engine/BasicShapes/Cube`, `/Engine/BasicShapes/Cylinder`, `/Engine/BasicShapes/Sphere` | Level references and runtime components; engine meshes are not copied.   |
| Editor-authored sky                                         | `/Engine/EngineSky/BP_Sky_Sphere`, `/Engine/EngineSky/SM_SkySphere`                      | Level reference only; engine assets are not copied into this repository. |
| Directional muzzle timing                                   | `/Niagara/DefaultAssets/Templates/Systems/DirectionalBurst`                              | `Content/Art/VFX/NS_MuzzleFlash.uasset`, with the CC0 Kenney sprite above |
| Projectile trail                                            | `/Niagara/DefaultAssets/Templates/Systems/AttributeReaderTrails`                         | `Content/Art/VFX/NS_ProjectileTrail.uasset`                              |
| Impact burst timing                                         | `/Niagara/DefaultAssets/Templates/Systems/RadialBurst`                                   | `Content/Art/VFX/NS_Impact.uasset`, with the CC0 Kenney sprite above      |
| Shield burst                                                | `/Niagara/DefaultAssets/Templates/Systems/RadialBurst`                                   | `Content/Art/VFX/NS_Shield.uasset`                                       |
| Destruction burst                                           | `/Niagara/DefaultAssets/Templates/Systems/SimpleExplosion`                               | `Content/Art/VFX/NS_Destruction.uasset`                                  |

Required Unreal notices:

> Automata War uses Unreal® Engine. Unreal® is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere.
>
> Unreal® Engine, Copyright 1998 - 2026, Epic Games, Inc. All rights reserved.

## Project-Generated Assets

The following assets are authored reproducibly by the scripts under `BuildScripts`, primarily `GenerateAssets.py`, `BuildItemAssets.py`, `BuildVFXAssets.py`, and `CreateArenaMap.py`. Imported third-party meshes remain itemized above.

- `Content/Art/Materials/M_ArenaCell.uasset`
- `Content/Art/Materials/M_Robot.uasset`
- `Content/Art/Materials/M_Cover.uasset`
- `Content/Art/Materials/M_Effect.uasset`
- `Content/Art/Materials/M_Ground.uasset`
- `Content/Art/Materials/M_Foliage.uasset`
- `Content/Art/Materials/M_Wood.uasset`
- `Content/Art/Materials/M_Stone.uasset`
- `Content/Art/Materials/M_Industrial.uasset`
- `Content/Art/VFX/Materials/M_VFX_Muzzle_Kenney.uasset`
- `Content/Art/VFX/Materials/M_VFX_Impact_Kenney.uasset`
- `Content/Blueprints/BP_AWPlayer.uasset`
- `Content/Blueprints/BP_AWPlayerController.uasset`
- `Content/Blueprints/BP_AWGameMode.uasset`
- `Content/Blueprints/BP_AWIsometricCamera.uasset`
- `Content/Blueprints/BP_AWArenaRenderer.uasset`
- `Content/Blueprints/Items/BP_ActionPointItem.uasset`
- `Content/Blueprints/Items/BP_ExtraAmmoItem.uasset`
- `Content/Blueprints/Items/BP_ShieldItem.uasset`
- `Content/Blueprints/Items/BP_AcceleratorItem.uasset`
- `Content/UI/WBP_AWHUD.uasset`
- `Content/UI/WBP_AWCursor.uasset`
- `Content/UI/T_AWCursorPixel.uasset`
- `Content/UI/WBP_AWCodeEditor.uasset`
- `Content/UI/Screens/WBP_AWMainMenuScreen.uasset`
- `Content/UI/Screens/WBP_AWProgrammingPanel.uasset`
- `Content/UI/Screens/WBP_AWProgrammingScreen.uasset`
- `Content/UI/Screens/WBP_AWSimulationDock.uasset`
- `Content/UI/Screens/WBP_AWSimulationScreen.uasset`
- `Content/UI/Screens/WBP_AWReplayAutopsyScreen.uasset`
- `Content/UI/Screens/WBP_AWReplayBrowserScreen.uasset`
- `Content/UI/Screens/WBP_AWLanguageReferenceScreen.uasset`
- `Content/Maps/L_AutomataArena.umap`

The imported tanks and all level dressing are presentation-only. Their transforms, materials, collision settings, and visual variants never feed back into the deterministic simulation.
