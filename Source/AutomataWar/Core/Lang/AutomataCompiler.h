#pragma once

/**
 * @file AutomataCompiler.h
 * @brief Tokenizer and compiler for Automata War assembly language.
 *
 * Compiles source text into fixed-size bytecode with full diagnostic reporting.
 * Engine-independent: no UObject, no Unreal types.
 */

#include "AutomataWar/Core/AutomataRules.h"
#include <string>
#include <vector>
#include <cstdint>

namespace Automata
{

// ─── Bytecode instruction ────────────────────────────────────────────────────

/**
 * @brief Fixed-size encoded instruction (8 bytes).
 *
 * Layout:
 *   [0]    opcode (Opcode enum)
 *   [1]    operandA (register index or flags)
 *   [2]    operandB (register index or CmpOp)
 *   [3]    reserved
 *   [4..5] imm16 (little-endian signed)
 *   [6..7] target (label-resolved PC offset, little-endian unsigned)
 */
struct Instruction
{
    Opcode  opcode    = Opcode::WAIT;
    uint8_t operandA  = 0;
    uint8_t operandB  = 0;
    uint8_t reserved  = 0;
    int16_t imm16     = 0;
    uint16_t target   = 0;
};
static_assert(sizeof(Instruction) == 8, "Instruction must be 8 bytes");

// ─── Compiled program ────────────────────────────────────────────────────────

/** Compiled bytecode program ready for the VM. */
struct Program
{
    std::vector<Instruction> code;
};

// ─── Diagnostics ─────────────────────────────────────────────────────────────

/** Severity of a compiler diagnostic. */
enum class DiagSeverity : uint8_t
{
    Error,
    Warning
};

/** Diagnostic category for programmatic matching. */
enum class DiagKind : uint8_t
{
    UnknownInstruction,
    UnknownLabel,
    DuplicateLabel,
    BadOperandCount,
    BadOperandType,
    ImmediateOutOfRange,
    SetToReadOnly,
    MalformedComparison,
    ProgramTooLong,
    SourceTooLong
};

/** A single compiler diagnostic with location. */
struct Diagnostic
{
    DiagSeverity severity = DiagSeverity::Error;
    DiagKind     kind     = DiagKind::UnknownInstruction;
    int32_t      line     = 0;   ///< 1-based line number.
    int32_t      column   = 0;   ///< 1-based column.
    std::string  message;        ///< Human-readable description.
    std::string  suggestion;     ///< Optional Levenshtein suggestion for unknown instructions.
};

/** Result of compilation. */
struct CompileResult
{
    Program                 program;
    std::vector<Diagnostic> diagnostics;

    /** True when zero errors (warnings allowed). */
    bool Ok() const;
};

// ─── Compiler entry point ────────────────────────────────────────────────────

/**
 * @brief Compile source text into bytecode.
 * @param source Full program text (may contain multiple lines).
 * @return CompileResult with bytecode (if no errors) and all diagnostics.
 *
 * Never crashes or asserts on hostile input.
 */
CompileResult Compile(const std::string& source);

} // namespace Automata
