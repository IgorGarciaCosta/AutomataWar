# Game Module

Unreal-facing match orchestration layer:

- **AWMatchTypes**: Shared command/phase enums, persistent effect state, result structs, and log categories.
- **AWGameMode**: Server-authoritative phase state machine (Programming→Submission→Simulation→ReplayAutopsy). Single `HandleSubmission` path for both local and RPC calls.
- **AWGameState**: Replicated phase, round, timer, command arrays, AP, persistent effects, outcome, and authoritative hash.
- **AWPlayerState**: Per-player slot assignment and submission status.
- **AWPlayerController**: Command-array submission RPC bridge.
- **AWGameSubsystem**: Public API for local matches, LAN sessions, replay CRUD, and status queries.
- **AWReplayService**: Disk-backed replay save/load/list/delete/import/export with filename sanitization.

Single-player planning lives under `Source/AutomataWar/AI`. `AAWAIController` never mutates match state; `AAWGameMode` submits its queue through `HandleSubmission` after slot 0 submits.
