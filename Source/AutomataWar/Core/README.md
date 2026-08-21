# Core

`Core/` is the deterministic headless match implementation. It runs without actors, physics, rendering, audio, or content assets.

- `Sim/` consumes finite `EAWCommand` arrays directly and owns canonical integer state.
- `Replay/` serializes command bytes plus seed and provides deterministic step navigation.
- `AutomataDomainTypes.h` owns reflected command, persistent-effect, and terminal-reason types without depending on the Game layer.
- `AutomataRules.h` contains the small balance surface, command limit, replay version, and ruleset hash.

The simulation never reads a `UWorld`, frame time, animation, physics, content, or global engine randomness.
