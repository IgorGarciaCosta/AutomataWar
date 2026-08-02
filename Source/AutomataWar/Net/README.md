# Net Module

Network layer for Automata War:

- **AWDesyncDetector**: Client-side re-simulation and hash comparison against server authority. Logs loudly via `LogAutomataNet` on mismatch but does not disconnect (server is always authoritative).

Session management lives in `AWGameSubsystem` (Game/) using OnlineSubsystem NULL for LAN/listen-server. Script submission validation uses `Server_SubmitScript` RPC flowing through the same `HandleSubmission` path as local calls.

Design: server never accepts client outcomes. Clients receive only scripts + seed + hash after simulation, then independently re-simulate to verify determinism.
