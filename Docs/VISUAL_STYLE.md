# AW-80 Phosphor Terminal

Automata War is presented as software running inside an AW-80 tactical terminal. The device is not a decorative overlay: its chassis, glass, status line, grid, and shutdown behavior are the game's visual frame on every screen.

## Design Rules

1. Use dark graphite hardware around green-black glass. The game viewport never touches the physical display edge.
2. Build layouts on a compact terminal grid. Prefer aligned labels, short uppercase commands, box-like divisions, and status readouts over floating cards.
3. Use phosphor green for primary information, amber for player two and caution, and red only for destructive actions or errors.
4. Keep text crisp. Scanlines and persistence should be visible at rest but never obscure characters, move hit targets, or create chromatic fringes.
5. Treat motion as hardware behavior. Transitions should resemble refresh, relay, or power events rather than modern easing decoration.

## Palette

The source of truth is `BuildScripts/BuildHUD.py`; values below are Unreal linear colors.

| Token | Value | Use |
| --- | --- | --- |
| Device chassis | `0.055, 0.060, 0.052` | Outer hardware |
| Device bezel | `0.018, 0.021, 0.018` | Recess around glass |
| Glass black | `0.002, 0.012, 0.007` | Screen background |
| Panel | `0.008, 0.028, 0.017` | Work surfaces |
| Phosphor | `0.180, 1.000, 0.420` | Primary actions and player one |
| Amber | `1.000, 0.620, 0.160` | Player two and caution |
| Alarm | `1.000, 0.240, 0.120` | Errors and destructive actions |
| Text | `0.720, 1.000, 0.780` | High-priority readable copy |
| Muted | `0.320, 0.520, 0.360` | Metadata and inactive state |

## Type And Layout

- Generated controls use Unreal's bundled monospaced Slate face with zero letter spacing. `F_AWMono.ttf` provides the OFL Roboto Mono source if a project composite-font asset is introduced later.
- `F_AWDisplay.ttf` provides the OFL Rajdhani source for a future product wordmark; operational surfaces remain monospaced.
- The historical baseline is the 80-column, 24-row terminal. The UI can scale responsively, but command data should still read as fixed-grid output.
- Controls use hard edges, one-pixel phosphor rails, stable dimensions, and uppercase verb labels.

## Motion And Effects

- Submit powers a programming panel down over 420 ms: vertical collapse consumes the first 68% of the transition, followed by horizontal collapse into a bright center line.
- `RETURN TO PLANNING` reverses the same transition and withdraws the authoritative submission.
- The shared glass uses sparse low-opacity scanlines. Avoid heavy barrel distortion, RGB separation, random glitch loops, and strong blur; these are television artifacts that compromise a text terminal's main strength.
- Brightness states, brief afterglow, reverse video, and status-line changes are preferred future effects.

## References

- [DEC VT100](https://en.wikipedia.org/wiki/VT100): 80x24 grid, 132-column mode, smooth scrolling, brightness states, reverse video, and box-drawing characters.
- [Monochrome monitor](https://en.wikipedia.org/wiki/Monochrome_monitor): P1 green and P3 amber phosphors, high text sharpness, persistence, ghosting, and brightness controls.
- [Duskers](https://store.steampowered.com/app/254320/Duskers/): command-line interaction presented as the player's operational connection to unreliable old technology.
- [Cool Retro Term](https://github.com/Swordfish90/cool-retro-term): restrained configurable color, font, glow, and cathode-display effects.
- [Alien: Isolation UI reference](https://www.gameuidatabase.com/gameData.php?id=381): cassette-futurist terminal framing and diegetic screen hierarchy.

No third-party visual asset was imported for this style. The chassis, rails, scanlines, palette, and animation are project-authored and generated reproducibly.