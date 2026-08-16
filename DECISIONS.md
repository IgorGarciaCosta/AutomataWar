# Automata War Decision Log

This log records decisions made where the build brief allowed or required engineering judgment.

## 2026-08-16 - Isolate the arena in a captured HUD feed and expose pickup presentation

**Decision:** Render the arena through a square orthographic scene-capture target used by the simulation and replay Widget Blueprints. Keep every surrounding gameplay region opaque, remove the replay scrubber and event dock, and keep transport actions together. Generate one data-only Blueprint for each concrete pickup and spawn those classes while retaining native fallbacks. Replace the muzzle and impact renderer materials with two CC0 Kenney Particle Pack sprites while retaining UE 5.5 Niagara template timing.

**Alternatives:** Continue revealing the player camera through transparent UMG gaps; duplicate item behavior in Blueprint graphs; import an account-gated Fab pack; author particle art from scratch.

**Rationale:** A scene capture makes the arena the only world-space image visible through the HUD. Data-only pickup Blueprints expose inherited components without splitting behavior ownership. The CC0 sprites provide a stylized, redistributable visual upgrade without an account-bound binary dependency. This supersedes the Epic-only muzzle and impact portion of the 2026-08-01 VFX decision.

## 2026-08-15 - Resolve complete-queue turns with AP-based round initiative

**Decision:** Treat one turn as one tank's complete command queue. Execute every command from the explicit round starter before executing every command from the opponent; retain one simulation snapshot per command for presentation and replay inspection. Round 1 selects the starter randomly on the server. Later rounds compare the AP balances carried from the previous round before programming costs are deducted; the higher balance starts, and equal AP uses a fresh random tie-break. Store the selected slot in replicated state and replay version 7.

**Alternatives:** Resolve one command from both tanks in every snapshot; alternate one command at a time between tanks; calculate initiative after command costs are paid.

**Rationale:** The queue is the player's complete programmed turn, so initiative determines which full plan resolves first. Per-command snapshots keep movement and VFX readable. Selecting before programming prevents players from changing initiative by deliberately spending AP, while explicit replication and replay storage preserve deterministic reconstruction.

## 2026-08-15 - Keep AI isolated by source boundary and reuse authoritative submission

**Decision:** Place `AAWAIController` and its difficulty enum under `Source/AutomataWar/AI`, but keep them in the existing runtime module. Generate deterministic AP-budgeted queues for slot 1 and submit them through `AAWGameMode::HandleSubmission` only after the human submits.

**Alternatives:** Add a second Unreal runtime module; let AI call the simulation directly; duplicate validation in the controller.

**Rationale:** The project remains small and already uses folder boundaries. A second module would add build/dependency plumbing without runtime isolation, while the shared submission path preserves the same AP, command, replay, and authority rules for every player type.

## 2026-08-15 - Make power-up duration canonical and replay-compatible

**Decision:** Store charged actions and timed pickup effects in `FAWRobotEffects`, replicate current and replay-start values, and encode them in replay version 5. Extra ammo adds 10 damage; temporary shields halve damage; pickup effects last two rounds including the collection round; shield effects do not stack beyond 50% reduction.

**Alternatives:** Treat effects as presentation-only; infer them from previous replay files; stack charged and temporary shields; add snapshots to replays.

**Rationale:** Cross-round behavior must be deterministic and reconstructible from compact replay inputs. Explicit state is smaller and clearer than replay snapshots, and non-stacking shields avoid an undocumented 75% reduction edge case.

## 2026-08-15 - Keep projectile timing and active indicators presentational

**Decision:** Resolve shots instantly in Core, then animate a slower bolt, growing beam, and Niagara trail to the recorded endpoint. Trigger impact, shield, and layered fire/smoke destruction feedback on arrival. Build the active-tank circle as a collision-free procedural annulus driven by replay step priority.

**Alternatives:** Slow deterministic simulation for projectile travel; use physics projectiles; use a square decal or HUD marker.

**Rationale:** Presentation timing must not change authoritative outcomes or hashes. Runtime components consume the existing event stream, and a world-space annulus remains legible around either level-authored tank without requiring another binary material asset.

## 2026-08-13 - Use a software terminal cursor and categorized UI audio

**Decision:** Register a pixel-stepped, green-outline `WBP_AWCursor` as the viewport software cursor for default, hand, and text-entry states. Store hover and pressed audio in each generated `FButtonStyle`, using five variants from the existing Kenney UI Audio CC0 pack. Remove the tank movement loop and its `S_Move` asset.

**Alternatives:** Replace the Windows hardware cursor; play one generic sound in every action handler; add a new audio pack; keep the movement asset but mute it.

**Rationale:** A software cursor scales with the UI and remains inside the game presentation. Native button-style sounds cover mouse, keyboard, and gamepad activation once, while categorized variants communicate intent without duplicate handler calls. Deleting movement audio prevents stale content from returning during replay.

## 2026-08-13 - Use a reusable programming panel and AW-80 terminal shell

**Decision:** Move each combatant command queue into `WBP_AWProgrammingPanel`, backed by one native `UAWProgrammingPanelWidget`. The panel owns commands, submission feedback, withdrawal, and a 420 ms CRT power-off transition. Present every HUD screen inside the shared AW-80 phosphor-terminal chassis generated by `BuildHUD.py`.

**Alternatives:** Keep duplicated player controls in the programming screen; implement animation in the root HUD; import a photographed CRT frame or a third-party terminal UI pack; apply a full-screen RGB distortion shader.

**Rationale:** One panel fixes both structural and behavioral duplication. A project-authored chassis, sparse scanlines, monospaced grid, green phosphor, amber secondary state, and red alarms establish a reproducible visual identity without adding licensed art or degrading text readability with heavy distortion.

## 2026-08-13 - Replace source programs with finite command queues

**Decision:** Remove Automata Lang, its compiler, bytecode, VM, registers, energy model, script validator, examples, syntax editor, and tick cap. Players now append `MOVE`, `FIRE`, `TURN LEFT`, or `TURN RIGHT` through buttons. Simulation consumes each command once and ends when both queues finish or a tank is destroyed.

**Alternatives:** Keep the compiler behind a button-generated source string; translate buttons into bytecode; retain the VM as a queue executor.

**Rationale:** All three alternatives preserve layers that no longer provide behavior. A shared reflected enum can cross UI, RPC, replication, replay, desync, and simulation boundaries directly. This decision supersedes the 2026-08-01 bytecode, VM intent, action tick-cost, and tick-capped replay decisions, plus the source-size and Slate code-editor decisions below.

## 2026-08-01 - Pin Unreal Engine 5.5

**Decision:** Bind `AutomataWar.uproject` to Unreal Engine 5.5 and verify against installed build 5.5.4, changelist 40574608.

**Alternatives:** Use the newest installed engine; leave the association implicit.

**Rationale:** The requested API and binary compatibility target is specifically UE 5.5.

## 2026-08-01 - Start from a minimal C++ project

**Decision:** Create one runtime module from scratch with no Starter Content and expose architectural layers as folders.

**Alternatives:** Use a gameplay template; create separate Unreal modules for every layer.

**Rationale:** A blank module avoids template noise. Folder boundaries keep the small project readable without adding module boilerplate, while `Core/` remains free of UObject/world dependencies.

## 2026-08-01 - Compile to fixed bytecode

**Decision:** Compile source once into fixed 8-byte instructions with resolved labels and a parallel source map.

**Alternatives:** Interpret source strings per tick; use a variable-width instruction stream.

**Rationale:** Fixed bytecode removes parsing and allocation from the simulation hot path and gives the debugger stable source locations.

## 2026-08-01 - Use intent -> validation -> effect

**Decision:** Each VM emits one intent; only the simulation can inspect arena state and apply effects.

**Alternatives:** Let the VM query or mutate world state directly.

**Rationale:** This is the sandbox boundary and makes standalone and server-authoritative online execution use the same path.

## 2026-08-01 - Integer-only canonical state

**Decision:** Store all canonical state as bounded integers and use an explicit xorshift PRNG.

**Alternatives:** Unreal vectors/physics; floating-point fixed-step simulation; engine-global RNG.

**Rationale:** Replays and input replication require platform-independent results. Rendering may interpolate with floats but cannot write back.

## 2026-08-01 - Effects begin on dispatch

**Decision:** An instruction emits its intent when dispatched, then remains busy for the rest of its tick cost.

**Alternatives:** Apply effects only after the busy duration.

**Rationale:** The model remains easy to inspect while preserving vulnerability windows for expensive actions.

## 2026-08-01 - Alternate resolution priority

**Decision:** Resolve player 1 first on even ticks and player 2 first on odd ticks.

**Alternatives:** Permanent player priority; simultaneous conflict rules for every effect.

**Rationale:** Alternation is deterministic, inexpensive, and fair over time.

## 2026-08-01 - Re-simulate replay navigation

**Decision:** Store no snapshots in replay files. Step-back and arbitrary seek re-simulate from tick zero.

**Alternatives:** Store every frame; periodic keyframes.

**Rationale:** The 1,800-tick cap makes re-simulation cheap and keeps replay payloads independent of match length.

## 2026-08-01 - Side-by-side local programming

**Decision:** Use a shared split editor for local play rather than hot-seat hiding.

**Alternatives:** Sequential hidden turns.

**Rationale:** It is faster to test and teach. Blind submission still applies to online play; local play is explicitly cooperative-device trust.

## 2026-08-01 - OnlineSubsystem NULL

**Decision:** Use a listen server with LAN discovery and direct IP through OnlineSubsystem NULL.

**Alternatives:** Steam, EOS, a custom relay, or a dedicated server.

**Rationale:** It meets zero-account LAN testing and keeps the authoritative data path visible.

## 2026-08-01 - Replicate inputs, not frames

**Decision:** Replicate accepted input, seed, outcome, and authoritative hash. Input was source text originally and is a command array after the 2026-08-13 decision.

**Alternatives:** Replicate robot transforms or per-tick snapshots.

**Rationale:** Input is tiny, deterministic state is reproducible, and the hash makes divergence loud.

## 2026-08-01 - Use editor-owned CC0 tank presentation with native fallbacks

**Decision:** Import two Quaternius CC0 tanks as static meshes selected on a renderer Blueprint. Keep the procedural robot assemblies as native fallbacks, and keep arena cells, cover, and projectiles snapshot-driven at runtime.

**Alternatives:** Ship only procedural primitives; add skeletal animation; make imported mesh placement part of simulation state.

**Rationale:** CC0 provenance is explicit, the combined static meshes are lightweight, and Blueprint defaults let designers replace presentation without recompiling or affecting deterministic state.

## 2026-08-01 - Generate materials in Unreal Python

**Decision:** Generate nine project materials through `BuildScripts/GenerateAssets.py`.

**Alternatives:** Hand-author binary assets; duplicate materials per player.

**Rationale:** The script is reproducible. One robot material exposes player color parameters instead of duplicating assets.

## 2026-08-01 - Derive VFX from Epic Niagara templates

**Decision:** Duplicate UE 5.5 DirectionalBurst, AttributeReaderTrails, RadialBurst, and SimpleExplosion systems into project-owned Niagara assets.

**Alternatives:** Empty systems; hand-built GPU emitters; downloaded effects.

**Rationale:** Engine templates are version-compatible, licensed for Unreal products, and produce real effects without an external dependency.

## 2026-08-01 - Source CC0 sound from Kenney

**Decision:** Use selected files from Kenney Sci-fi Sounds and UI Audio packs.

**Alternatives:** Freesound CC-BY assets; generated tones; no audio.

**Rationale:** CC0 provenance is clear and the packs match the readable stylized direction.

## 2026-08-01 - Use OFL fonts from Google Fonts

**Decision:** Use Roboto Mono for code/debug text and Rajdhani SemiBold for display text.

**Alternatives:** Engine default fonts; a single typeface.

**Rationale:** Roboto Mono improves code alignment, while Rajdhani gives headings a technical identity. Both are redistributable under OFL 1.1.

## 2026-08-01 - Keep UI behavior native and expose editor-owned Widget Blueprints

**Decision:** Build the six UI screens and syntax highlighter in C++ Slate, then expose Blueprintable `UUserWidget` parents and thin Widget Blueprint assets. Superseded for screen composition by the 2026-08-02 decision below; the code editor remains Slate-backed.

**Alternatives:** Reimplement behavior in UMG Blueprint graphs; use web-based editor integration; keep all UI class selection hard-coded.

**Rationale:** Slate retains deterministic ownership over editor spans, gutter lines, and diagnostics, while Widget Blueprints provide designer-visible class assets without duplicating logic.

## 2026-08-02 - Author screens and tanks as editor-owned assets

**Decision:** Store the full HUD hierarchy and styling in the Canvas-based `WBP_AWHUD`, keep only behavior and bindings in `UAWHUDWidget`, and represent each simulated robot with a level-authored `BP_TankActor` derived from `AAWTankActor`. Context screens are fully opaque; simulation and replay use opaque edge panels around a transparent central arena viewport viewed by an orthographic top-down camera. Arena boundary walls belong to the map rather than `AAWArenaRenderer`.

**Alternatives:** Keep screen composition in `RebuildWidget`; retain both tank meshes as renderer components; continue generating boundary cubes from simulation wall cells.

**Rationale:** Designers can edit and preview the menus directly, each tank has an explicit Unreal lifecycle and map identity, and the renderer has a smaller presentation-only responsibility without duplicating walls already present in the level.

## 2026-08-07 - Split the HUD into modular screen Widget Blueprints

**Decision:** Keep `WBP_AWHUD` as the navigation and status shell, but move each switcher child into its own Widget Blueprint under `Content/UI/Screens`. Native screen classes own their control bindings and emit semantic actions; `UAWHUDWidget` coordinates navigation and game data without reaching into child widget trees.

**Alternatives:** Keep the full six-screen tree in one Widget Blueprint; wrap each screen without moving bindings; move all orchestration into Blueprint graphs.

**Rationale:** Each screen can be edited, compiled, and extended independently. Typed screen APIs preserve native behavior while removing the root HUD's dependency on internal control names.

## 2026-08-02 - Keep local mode in the existing world

**Decision:** Reset the active standalone `AAWGameMode` instead of reopening the arena with `?listen`.

**Alternatives:** Map travel for every local match.

**Rationale:** Travel destroyed the newly selected programming screen and changed net mode. The default arena is already loaded, so resetting authoritative state is both simpler and correct.

## 2026-08-02 - Guarantee replay size at the trust boundary

**Decision:** Limit each submitted script to 1,800 UTF-8 bytes/characters.

**Alternatives:** Allow 8 KiB scripts and call only typical replays small; add compression.

**Rationale:** Two maximum scripts plus all binary replay metadata total 3,630 bytes, making the under-4-KiB property testable and unconditional.

## 2026-08-02 - Non-shipping deterministic capture mode

**Decision:** Support `-AutomataCapture=Programming` and `-AutomataCapture=Replay` only in non-shipping builds.

**Alternatives:** Manually stage screenshots; add a permanent demo menu.

**Rationale:** Documentation images and startup smoke tests can drive the real production handlers without brittle desktop input or shipping debug controls.

## Blueprint Usage

No gameplay or UI logic uses Blueprint. The project map and generated binary assets exist because Unreal serializes maps, materials, Niagara systems, and imported sounds/fonts as assets; all behavior remains in C++.
