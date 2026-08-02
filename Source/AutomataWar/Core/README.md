# Core

`Core/` is the engine-independent deterministic product. It can compile and run a complete match without actors, physics, rendering, audio, or content assets.

- `Lang/` validates source and emits fixed 8-byte bytecode plus source locations.
- `VM/` advances one sandboxed robot and emits arena-agnostic intents.
- `Sim/` owns canonical integer state and applies intent -> validation -> effect.
- `Replay/` serializes scripts plus seed and provides deterministic navigation.
- `AutomataRules.h` is the single source of truth for balance, opcodes, registers, limits, replay version, and ruleset hash.

The boundary is one-way: Unreal gameplay and presentation code may consume Core results; Core never includes UObject/world, frame-time, animation, physics, content, or global engine-randomness APIs.