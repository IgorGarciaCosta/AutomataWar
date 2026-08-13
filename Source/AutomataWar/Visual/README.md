# Visual Layer

C++ presentation-only visuals for Automata War. Reads simulation snapshots; never writes to Core.

## Structure

- `AWVisualTypes.h/cpp` — Log category, asset soft paths, visual config constants
- `AWArenaRenderer.h/cpp` — Floor grid, dynamic cover, shot/VFX/audio coordination
- `AWTankActor.h/cpp` — Level-authored tank mesh, accent light, and snapshot interpolation
- `AWIsometricCamera.h/cpp` — Fixed orthographic top-down camera framing the arena inside the HUD viewport
- `AWSpectatorPawn.h/cpp` — Minimal pawn (no movement, no collision)

## Design

- All collision disabled; no physics involved in gameplay
- Two `BP_TankActor` instances, derived from `AAWTankActor`, are placed in `L_AutomataArena` and referenced by the renderer
- Arena boundary walls are map-authored; the renderer does not generate `CellType::Wall` cubes
- Player-color: cyan (P1) vs coral (P2) with emissive differentiation
- Cover variants chosen deterministically from cell index (3 visual styles)
- VFX: references optional Niagara systems via soft paths; complete emissive/point-light fallback
- Audio: soft-path references; silent when assets absent
- Interpolation uses frame delta purely for visual smoothness
