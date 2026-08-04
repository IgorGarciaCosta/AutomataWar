# UI Layer

Designer-authored UMG presentation for Automata War with native C++ behavior.

## Structure

- `AWUITypes.h/cpp` — Log category, color palette, asset soft paths
- `SAWCodeEditor.h/cpp` — Slate multiline code editor with syntax highlighting, line numbers, diagnostics
- `SAWSyntaxHighlighter.h/cpp` — Rich-text marshaller for Automata assembly coloring
- `AWCodeEditorWidget.h/cpp` — UMG host for the native Slate code editor
- `AWHUDWidget.h/cpp` — Runtime actions and data bindings for the root HUD
- `WBP_AWHUD` — Canvas-based responsive layout, styling, and six-screen hierarchy

## Screens

1. **Main Menu** — Local Match, Host LAN, Find LAN, Join IP, Replay Browser, Language Reference, Quit
2. **Programming** — Split editors for P1/P2, example loaders, submit/lock, training bot
3. **Simulation** — Framed arena viewport with locked source panels and match status
4. **Replay Autopsy** — Framed arena viewport, source/register panels, playback controls, event log, next round
5. **Replay Browser** — Lists Saved/Replays, load/watch, base64 export/import
6. **Language Reference** — Auto-generated from Core definitions (8 instructions, 9 registers)

Main Menu, Programming, Replay Browser, and Language Reference use opaque full-screen backdrops. Simulation and Replay Autopsy expose the arena only through their central framed viewport.
