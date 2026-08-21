# Game Module

Unreal-facing match orchestration layer:

- **AWMatchTypes**: Shared command/phase enums, persistent effect state, result structs, and log categories.
- **AWGameMode**: Server-authoritative phase state machine (Programming→Submission→Simulation→ReplayAutopsy). Single `HandleSubmission` path for both local and RPC calls.
- **AWGameState**: Replicated phase, timer, live AP/effects, and one atomic resolved-round record for replay and outcome data.
- **AWPlayerState**: Replicated per-player command-slot assignment.
- **AWPlayerController**: Sole command submission/withdrawal gateway for local slots and network RPCs.
- **AWGameSubsystem**: Persistent API for local match setup, LAN sessions, and replay CRUD.
- **AWReplayService**: Disk-backed replay save/load/list/delete/import/export with filename sanitization.

Single-player planning lives under `Source/AutomataWar/AI`. The stateless planner never mutates match state; `AAWGameMode` submits its queue through `HandleSubmission` after slot 0 submits.
