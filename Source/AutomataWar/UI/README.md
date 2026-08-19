# UI Layer

Designer-authored UMG presentation for Automata War with native C++ behavior.

## Structure

- `AWUITypes.h/cpp` — Log category, color palette, asset soft paths
- `AWScreenWidget.h/cpp` — Typed screens, command queues, button bindings, and semantic UI events
- `AWHUDWidget.h/cpp` — Root coordinator for navigation, game state, and screen data
- `WBP_AWHUD` — Responsive shell, status bar, and six-screen switcher
- `Content/UI/Screens` — Independently editable screen Widget Blueprints and reusable simulation dock

## Screens

1. **Main Menu** — Single Player, Local Versus, LAN, replays, command reference, quit
2. **Difficulty** — Easy, Normal, or Hard AI planning depth
3. **Arena Selection** — Compact, standard, or expanded procedural combat grid
4. **Replay Autopsy** — Persistent arena viewport with programming bays, replay docks, playback controls, and next round
5. **Replay Browser** — Lists Saved/Replays, load/watch, base64 export/import
6. **Command Reference** — Describes all seven available commands

Main Menu, setup, Replay Browser, and Language Reference use opaque full-screen backdrops. Replay Autopsy exposes the arena only through its central framed viewport while both planning and playback remain on that screen.
