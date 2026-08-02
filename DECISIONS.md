# Automata War Decision Log

This log records decisions made where the build brief allowed or required engineering judgment.

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

**Decision:** Replicate accepted source, seed, outcome, and authoritative hash.

**Alternatives:** Replicate robot transforms or per-tick snapshots.

**Rationale:** Input is tiny, deterministic state is reproducible, and the hash makes divergence loud.

## 2026-08-01 - Assemble readable robot art procedurally

**Decision:** Build two distinct robot silhouettes, three cover variants, arena cells, and projectiles at runtime from Unreal basic meshes. Apply project-authored parameterized materials.

**Alternatives:** Download third-party robot packs; import Blender-generated meshes; ship default grey primitives.

**Rationale:** Procedural assemblies give exact facing silhouettes, tiny repository size, deterministic placement, and no uncertain model license. Deliberate materials and multi-part silhouettes provide a finished visual identity.

## 2026-08-01 - Generate materials in Unreal Python

**Decision:** Generate four project materials through `BuildScripts/GenerateAssets.py`.

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

## 2026-08-01 - Use C++ Slate instead of Blueprint widgets

**Decision:** Build the six UI screens and syntax highlighter in C++ Slate.

**Alternatives:** UMG Blueprint graphs; web-based editor integration.

**Rationale:** The brief prioritizes C++, and Slate provides deterministic ownership over editor spans, gutter lines, and diagnostic state without Blueprint logic.

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