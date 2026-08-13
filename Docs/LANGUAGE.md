# Command Reference

Automata War no longer accepts source code. Players construct a finite ordered queue using four command buttons.

## Execution

1. Each action step gives each living tank at most one command.
2. First-player priority alternates each step for deterministic fairness.
3. A tank whose queue has ended performs no action.
4. Commands never wrap or repeat.
5. The round ends when both queues are exhausted or either tank reaches zero HP.

## `MOVE`

Move one cell forward relative to the tank's current facing. Arena walls, cover, and the opposing tank block the move. A blocked move still consumes that queue entry.

## `FIRE`

Trace forward from the cannon along the tank's current facing. The first wall, cover cell, or opposing tank ends the shot. Cover loses 20 health; a tank hit loses 20 HP.

## `TURN LEFT`

Rotate 90 degrees counterclockwise from the tank's point of view. For example, a south-facing tank turns left to face east.

## `TURN RIGHT`

Rotate 90 degrees clockwise from the tank's point of view. For example, a north-facing tank turns right to face east.

## Limits

- At least one command is required for submission.
- Each player may submit at most 256 commands.
- RPC, replicated, and replay values use the reflected `EAWCommand` enum.
- Invalid enum values are rejected by the server and replay decoder.