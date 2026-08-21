# Tests

`Tests/` contains Unreal Automation coverage for the engine-independent Core and its Unreal integration boundaries.

- `AutomataCoreTests.cpp`: seven-command execution, relative turns, deterministic hashes, persistent state, effects, and replay integrity.
- `AWGameTests.cpp`: submission state, deterministic AI, resolved-round conversion, replay storage bounds, command labels, and desync verification.
- `AWPresentationTests.cpp`: plan projection, replay navigation, HUD contracts, result timing/copy, typewriter frames, and production VFX assets.

Tests may instantiate lightweight Unreal objects where required, but the headless simulation test deliberately loads no presentation asset. Run the complete group with `Automation RunTests AutomataWar`.
