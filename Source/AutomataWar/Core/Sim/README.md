# Sim — Automata Simulation

Deterministic headless match simulation. Integer-only logic, seeded xorshift PRNG, fixed-capacity arrays.
Executes each finite command queue once as two complete turns. The configured starter runs its full queue first, then the opponent runs its full queue; each snapshot contains exactly one active tank command.
Produces event logs and per-step snapshots with a canonical 64-bit state hash.
