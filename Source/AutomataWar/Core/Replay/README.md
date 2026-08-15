# Replay — Automata Replay Codec

Compact binary codec storing both finite command arrays, round starter, seed, version, ruleset hash, and CRC-32.
No simulation snapshots — matches are re-simulated from the replay payload.
Supports base64 import/export with CRC-32 integrity check.
