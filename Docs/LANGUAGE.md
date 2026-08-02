# Automata Lang Specification

Automata Lang v1 is a case-insensitive, line-oriented assembly language compiled to fixed 8-byte bytecode before a match starts.

## Lexical Rules

- Source is UTF-8 at the replay boundary and validated as printable ASCII plus tab, LF, and CR for network submission.
- Spaces, tabs, and commas separate tokens.
- `;` and `//` start a comment that continues to end of line.
- Mnemonics, registers, `JUMP`, and labels are case-insensitive.
- Blank and comment-only lines emit no instruction.
- A label ends in `:` and resolves to the next emitted instruction.
- Immediate integers are decimal values in `[-32768, 32767]`.
- A program may contain at most 256 emitted instructions and 512 source lines.
- A network-submitted source may contain at most 1,800 bytes, 1,800 characters, and 32 characters per token.

## Grammar

```ebnf
program        = { line } ;
line           = spacing, { label, spacing }, [ instruction ],
                 spacing, [ comment ], newline ;

label          = identifier, ":" ;
identifier     = ( letter | "_" ), { letter | digit | "_" } ;

instruction    = move | turn | scan | fire | shield | set | branch | wait ;
move           = "MOVE", spacing1, ( "FWD" | "BACK" ) ;
turn           = "TURN", spacing1, ( "LEFT" | "RIGHT" ) ;
scan           = "SCAN" ;
fire           = "FIRE" ;
shield         = "SHIELD" ;
set            = "SET", spacing1, writable-register, spacing1, immediate ;
branch         = "IF", spacing1, register, spacing1, comparison,
                 spacing1, immediate, spacing1, "JUMP", spacing1, identifier ;
wait           = "WAIT" ;

writable-register = "R0" | "R1" | "R2" | "R3" ;
register       = writable-register | "R_HP" | "R_ENEMY_DIST" |
                 "R_ENEMY_DIR" | "R_ENERGY" | "R_TICK" ;
comparison     = "==" | "!=" | "<" | ">" | "<=" | ">=" ;
immediate      = [ "-" ], digit, { digit } ;
comment        = ";", { character } | "//", { character } ;
spacing        = { " " | tab | "," } ;
spacing1       = ( " " | tab | "," ), spacing ;
newline        = LF | CR, LF | end-of-file ;
letter         = "A".."Z" | "a".."z" ;
digit          = "0".."9" ;
```

The compiler accepts multiple leading labels on one source line. Labels should follow `identifier` syntax; the server token-length and character checks are applied before compilation.

## Execution Model

Each robot owns:

- Program counter (`PC`).
- General registers `R0..R3`.
- System registers.
- Energy.
- A busy counter.
- An executed-instruction counter.

At most one instruction is dispatched by each robot per simulation tick. Dispatch emits an intent immediately. The simulation validates the intent against canonical arena state, applies its effect, and leaves the VM busy for the remaining action ticks. A busy VM emits no new intent.

When `PC` reaches the bytecode length, it wraps to instruction zero. An empty program or invalid out-of-range `PC` halts safely. Energy exhaustion makes the robot inert: it can produce only `WAIT` behavior.

The global match cap is 1,800 ticks. Resolution priority alternates by tick so neither player owns permanent first action.

## Instructions

### `MOVE <FWD|BACK>`

- Tick cost: 2
- Energy cost: 2
- Computes one cardinal cell relative to facing.
- A wall, cover cell, or the other robot blocks movement.
- A blocked action still consumes its full cost.
- Replay events distinguish wall, cover, and robot blockage.

### `TURN <LEFT|RIGHT>`

- Tick cost: 1
- Energy cost: 1
- Rotates facing by 90 degrees.

### `SCAN`

- Tick cost: 1
- Energy cost: 3
- Tests the enemy against a forward 90-degree cone with range 8.
- Cover blocks line of sight.
- A hit writes Manhattan distance to `R_ENEMY_DIST` and relative direction to `R_ENEMY_DIR`.
- Relative direction is `-1` left, `0` ahead, or `1` right.
- A miss writes `R_ENEMY_DIST = 0`.

### `FIRE`

- Tick cost: 4
- Energy cost: 12
- Creates a projectile at the robot position along current facing.
- A projectile advances up to 4 cells per simulation tick.
- Cover and arena bounds destroy it.
- A robot hit deals 20 damage unless shielded.
- The simulation owns a fixed pool of at most 16 active projectiles.

### `SHIELD`

- Tick cost: 3
- Energy cost: 15
- Activates a one-hit shield.
- The next incoming hit is absorbed completely and consumes the shield.
- The robot remains busy for the full action cost.

### `SET <Rn> <imm>`

- Tick cost: 1
- Energy cost: 0
- Writes the immediate to one of `R0..R3`.
- System registers are read-only and rejected at compile time.

### `IF <reg> <OP> <imm> JUMP <label>`

- Tick cost: 1
- Energy cost: 0
- Reads any general or system register.
- Compares it to an immediate using `==`, `!=`, `<`, `>`, `<=`, or `>=`.
- A true condition sets `PC` to the resolved label index.
- A false condition continues to the next instruction.
- Text aliases such as `EQ` and `GOTO` are intentionally rejected.

### `WAIT`

- Tick cost: 1
- Energy cost: 0
- Emits no gameplay effect.
- Useful for conserving energy and changing timing.

## Registers

| Name | Access | Update rule |
| --- | --- | --- |
| `R0` | Read/write | Initialized to `0`; changed only by `SET`. |
| `R1` | Read/write | Initialized to `0`; changed only by `SET`. |
| `R2` | Read/write | Initialized to `0`; changed only by `SET`. |
| `R3` | Read/write | Initialized to `0`; changed only by `SET`. |
| `R_HP` | Read-only | Synchronized from current HP after effects. |
| `R_ENEMY_DIST` | Read-only | Written by `SCAN`; `0` means no target. |
| `R_ENEMY_DIR` | Read-only | Written by successful `SCAN`. |
| `R_ENERGY` | Read-only | Synchronized from remaining energy. |
| `R_TICK` | Read-only | Synchronized from current simulation tick. |

## Numeric and Safety Behavior

There is no arithmetic instruction in v1. User-controlled integers enter only through 16-bit-range immediates, and all system values are bounded by centralized rules. Program indices are unsigned 16-bit values but programs are capped at 256 instructions. Match and instruction counters are bounded by the 1,800-tick cap. Invalid `PC` state halts rather than indexing memory.

The simulation performs no dynamic allocation in its per-tick intent/effect path. Projectiles use a fixed array. Infinite behavior loops consume simulation ticks and terminate at the global cap.

## Diagnostics

Diagnostics include a 1-based line and column. Compilation is panic-free and reports multiple independent errors when possible.

| Kind | Trigger | Example message |
| --- | --- | --- |
| `UnknownInstruction` | Mnemonic is not one of eight instructions. | `Unknown instruction 'FIREE'. Did you mean 'FIRE'?` |
| `UnknownLabel` | Branch target was never declared. | `Unknown label 'ATTACK'.` |
| `DuplicateLabel` | A label is declared more than once. | `Duplicate label 'LOOP'.` |
| `BadOperandCount` | Instruction has too few or too many operands. | `MOVE takes 1 operand (FWD or BACK), got 0.` |
| `BadOperandType` | Register, direction, or integer token is invalid. | `Expected integer immediate, got 'FAST'.` |
| `ImmediateOutOfRange` | Immediate is outside the signed 16-bit range. | `Immediate 40000 outside [-32768, 32767].` |
| `SetToReadOnly` | `SET` targets a system register. | `Cannot SET system read-only register 'R_HP'.` |
| `MalformedComparison` | `IF` uses an unknown symbolic operator. | `Unknown comparison operator '~'. Use == != < > <= >=.` |
| `ProgramTooLong` | More than 256 instructions are emitted. | `Program exceeds maximum of 256 instructions.` |
| `SourceTooLong` | More than 512 lines reach the compiler. | `Source exceeds maximum of 512 lines.` |
| `MissingJumpKeyword` | Token five of `IF` is not `JUMP`. | `Expected JUMP keyword, got 'THEN'.` |
| `AliasRejected` | `EQ/NE/LT/LE/GT/GE` or `GOTO` is used. | `GOTO rejected. Use JUMP.` |

The suggestion engine computes case-insensitive Levenshtein distance against the eight mnemonics and offers the nearest name when distance is at most 3.

Before compilation, the authoritative server separately rejects oversized source, too many lines, tokens longer than 32 characters, and characters outside printable ASCII/tab/newline/carriage return.

## Bytecode Layout

Each instruction is exactly 8 bytes:

```text
byte 0      opcode
byte 1      operand A (register or direction)
byte 2      operand B (comparison)
byte 3      reserved
bytes 4-5   signed immediate, little-endian
bytes 6-7   resolved instruction target, little-endian
```

A parallel source map stores the original line and column for each emitted instruction. It is used by the replay debugger and is not part of canonical simulation state.