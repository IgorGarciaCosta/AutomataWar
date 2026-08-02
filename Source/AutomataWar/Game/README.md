# Game Module

Unreal-facing match orchestration layer:

- **AWMatchTypes**: Shared enums (EAWMatchPhase), structs, log categories (LogAutomataGame, LogAutomataNet).
- **AWGameMode**: Server-authoritative phase state machine (Programming→Submission→Simulation→ReplayAutopsy). Single `HandleSubmission` path for both local and RPC calls.
- **AWGameState**: Replicated phase, round, timer, revealed scripts, outcome, authoritative hash.
- **AWPlayerState**: Per-player slot assignment and submission status.
- **AWPlayerController**: Script submission RPC bridge.
- **AWGameSubsystem**: Public BlueprintCallable API for local match, hosting, LAN discovery, join, training, replay CRUD, status queries.
- **AWScriptValidator**: Source size/charset/structure validation.
- **AWExampleScripts**: Three example bots (Aggressor, Camper, Kiter) + DefaultBot.
- **AWReplayService**: Disk-backed replay save/load/list/delete/import/export with filename sanitization.
