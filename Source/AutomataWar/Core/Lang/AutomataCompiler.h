#pragma once

/**
 * @file AutomataCompiler.h
 * @brief Tokenizer and compiler for Automata War assembly language.
 *
 * Engine-independent: no UObject, no Unreal types.
 */

#include "AutomataWar/Core/AutomataRules.h"
#include <string>
#include <vector>
#include <cstdint>

namespace Automata
{

    // --- Bytecode instruction (8 bytes fixed) -----------------------------------

    /**
     * Layout:
     *   [0]    opcode (Opcode enum)
     *   [1]    operandA (register index or MoveDir or turn direction)
     *   [2]    operandB (CmpOp for IF)
     *   [3]    reserved
     *   [4..5] imm16 (little-endian signed)
     *   [6..7] target (label-resolved instruction index, little-endian unsigned)
     */
    struct Instruction
    {
        Opcode opcode = Opcode::WAIT;
        uint8_t operandA = 0;
        uint8_t operandB = 0;
        uint8_t reserved = 0;
        int16_t imm16 = 0;
        uint16_t target = 0;
    };
    static_assert(sizeof(Instruction) == 8, "Instruction must be 8 bytes");

    // --- Source map entry --------------------------------------------------------

    struct SourceLocation
    {
        int32_t line = 0;   // 1-based
        int32_t column = 0; // 1-based
    };

    // --- Compiled program --------------------------------------------------------

    struct Program
    {
        std::vector<Instruction> code;
        std::vector<SourceLocation> sourceMap; // parallel to code: source origin per instruction
    };

    // --- Diagnostics -------------------------------------------------------------

    enum class DiagSeverity : uint8_t
    {
        Error,
        Warning
    };

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
        SourceTooLong,
        MissingJumpKeyword,
        AliasRejected
    };

    struct Diagnostic
    {
        DiagSeverity severity = DiagSeverity::Error;
        DiagKind kind = DiagKind::UnknownInstruction;
        int32_t line = 0;
        int32_t column = 0;
        std::string message;
        std::string suggestion;
    };

    struct CompileResult
    {
        Program program;
        std::vector<Diagnostic> diagnostics;
        bool Ok() const;
    };

    // --- Compiler entry point ----------------------------------------------------

    CompileResult Compile(const std::string &source);

} // namespace Automata
