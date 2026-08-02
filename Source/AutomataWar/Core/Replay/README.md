# Replay — Automata Replay Codec

Compact binary codec storing both script sources, seed, version, and ruleset hash.
No simulation snapshots — matches are re-simulated from the replay payload.
Supports base64 import/export with CRC-32 integrity check.
