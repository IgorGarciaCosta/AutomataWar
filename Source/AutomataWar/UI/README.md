# UI Layer

C++ presentation layer for Automata War. All screens built in Slate/UMG without Blueprints.

## Structure

- `AWUITypes.h/cpp` — Log category, color palette, asset soft paths
- `SAWCodeEditor.h/cpp` — Slate multiline code editor with syntax highlighting, line numbers, diagnostics
- `SAWSyntaxHighlighter.h/cpp` — Rich-text marshaller for Automata assembly coloring
- `AWHUDWidget.h/cpp` — Root UMG widget managing all UI screens (menu, editors, replay, reference)

## Screens

1. **Main Menu** — Local Match, Host LAN, Find LAN, Join IP, Replay Browser, Language Reference, Quit
2. **Programming** — Split editors for P1/P2, example loaders, submit/lock, training bot
3. **Simulation** — Wait indicator
4. **Replay Autopsy** — Playback controls (pause/play/step/scrub/speeds), next round
5. **Replay Browser** — Lists Saved/Replays, load/watch, base64 export/import
6. **Language Reference** — Auto-generated from Core definitions (8 instructions, 9 registers)
