# Visual Layer

C++ presentation-only visuals for Automata War. Reads simulation snapshots; never writes to Core.

## Structure

- `AWVisualTypes.h/cpp` — Log category, asset soft paths, visual config constants
- `AWArenaRenderer.h/cpp` — Floor grid, dynamic cover, shot/VFX/audio coordination
- `AWTankActor.h/cpp` — Level-authored tank mesh, snapshot interpolation, and active-turn ring
- `AWItem.h/cpp` — Shared pickup presentation plus ammo, shield, and accelerator actors
- `AWAPItem.h/cpp` — AP-specific pickup identity derived from `AAWItem`
- `AWIsometricCamera.h/cpp` — Fixed orthographic scene capture supplying the square HUD arena feed
- `AWSpectatorPawn.h/cpp` — Minimal pawn (no movement, no collision)

## Design

- All collision disabled; no physics involved in gameplay
- Two `BP_TankActor` instances, derived from `AAWTankActor`, are placed in `L_AutomataArena` and referenced by the renderer
- Four data-only pickup Blueprints expose the inherited mesh, trigger, and light components while native classes retain behavior; downloaded CC0 meshes provide bullet, rocket, coin, and shield silhouettes
- Arena boundary walls are map-authored; the renderer does not generate `CellType::Wall` cubes
- Player-color: cyan (P1) vs coral (P2) with emissive differentiation
- Cover variants chosen deterministically from cell index (3 visual styles)
- Muzzle feedback uses Kopo Study's packaged system with a hard 0.2-second lifetime cap
- Projectile impact uses UE's finite SimpleExplosion system for a lightweight explosion-and-smoke burst
- Audio: soft-path references; silent when assets absent
- Interpolation uses frame delta purely for visual smoothness
- Projectile gameplay resolves instantly in Core; presentation animates a slower bolt, Niagara trail, and growing beam before impact feedback
- Active shields use a state-owned emissive energy sphere and vanish on one-shot absorption or timed-effect expiry
- `NS_Destruction` supplies the larger destruction burst, layered with short-lived orange lights
- The active-tank annulus is procedural, collision-free, and follows the snapshot's sole active command
