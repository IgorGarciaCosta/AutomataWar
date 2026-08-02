# Visual Layer

C++ presentation-only visuals for Automata War. Reads simulation snapshots; never writes to Core.

## Structure

- `AWVisualTypes.h/cpp` — Log category, asset soft paths, visual config constants
- `AWArenaRenderer.h/cpp` — Floor grid, cover blocks, robot composites, projectile/VFX/audio
- `AWIsometricCamera.h/cpp` — Fixed isometric camera framing the arena
- `AWSpectatorPawn.h/cpp` — Minimal pawn (no movement, no collision)

## Design

- All collision disabled; no physics involved in gameplay
- Robots built from engine mesh primitives (Cube/Cylinder) with dynamic materials
- Player-color: cyan (P1) vs coral (P2) with emissive differentiation
- Cover variants chosen deterministically from cell index (3 visual styles)
- VFX: references optional Niagara systems via soft paths; complete emissive/point-light fallback
- Audio: soft-path references; silent when assets absent
- Interpolation uses float/Tick purely for visual smoothness
