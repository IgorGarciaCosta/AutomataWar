# Net Module

Network layer for Automata War:

- **AWDesyncDetector**: Client-side re-simulation and hash comparison against server authority. Logs loudly via `LogAutomataNet` on mismatch but does not disconnect (server is always authoritative).

Session management lives in `AWGameSubsystem` (Game/) using OnlineSubsystem NULL for LAN/listen-server. Command submission uses the `Server_SubmitCommands` RPC and the same bounded `HandleSubmission` path as local play.

Design: the server never accepts client outcomes. Clients receive only command arrays, seed, and hash after simulation, then independently re-simulate to verify determinism.
