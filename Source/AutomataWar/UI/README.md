# UI Layer

Designer-authored UMG presentation for Automata War with native C++ behavior.

## Structure

- `AWUITypes.h/cpp` — Log category, color palette, asset soft paths
- `AWScreenWidget.h/cpp` — Typed screens, command queues, button bindings, and semantic UI events
- `AWHUDWidget.h/cpp` — Root coordinator for navigation, game state, and screen data
- `WBP_AWHUD` — Responsive shell, status bar, and seven-screen switcher
- `Content/UI/Screens` — Independently editable screen Widget Blueprints and reusable simulation dock

## Screens

1. **Main Menu** — Single Player, Local Versus, LAN, replays, command reference, quit
2. **Difficulty** — Easy, Normal, or Hard AI planning depth
3. **Programming** — Split action queues; slot 1 becomes read-only in single player
4. **Simulation** — Framed arena viewport with locked command panels and match status
5. **Replay Autopsy** — Framed arena viewport, playback controls, event log, next round
6. **Replay Browser** — Lists Saved/Replays, load/watch, base64 export/import
7. **Command Reference** — Describes all seven available commands

Main Menu, Programming, Replay Browser, and Language Reference use opaque full-screen backdrops. Simulation and Replay Autopsy expose the arena only through their central framed viewport.
