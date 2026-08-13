# UI Layer

Designer-authored UMG presentation for Automata War with native C++ behavior.

## Structure

- `AWUITypes.h/cpp` — Log category, color palette, asset soft paths
- `AWScreenWidget.h/cpp` — Typed screens, command queues, button bindings, and semantic UI events
- `AWHUDWidget.h/cpp` — Root coordinator for navigation, game state, and screen data
- `WBP_AWHUD` — Responsive shell, status bar, and six-screen switcher
- `Content/UI/Screens` — Independently editable screen Widget Blueprints and reusable simulation dock

## Screens

1. **Main Menu** — Local Match, Host LAN, Find LAN, Join IP, Replay Browser, Language Reference, Quit
2. **Programming** — Split action queues with scrollable Available Commands rails and remove/submit controls
3. **Simulation** — Framed arena viewport with locked command panels and match status
4. **Replay Autopsy** — Framed arena viewport, highlighted command panels, playback controls, event log, next round
5. **Replay Browser** — Lists Saved/Replays, load/watch, base64 export/import
6. **Command Reference** — Describes the four available tank-relative commands

Main Menu, Programming, Replay Browser, and Language Reference use opaque full-screen backdrops. Simulation and Replay Autopsy expose the arena only through their central framed viewport.
