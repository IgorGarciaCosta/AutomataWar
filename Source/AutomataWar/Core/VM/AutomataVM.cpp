/**
 * @file AutomataVM.cpp
 * @brief Implementation of the Automata War virtual machine.
 */

#include "AutomataVM.h"

namespace Automata
{

Intent VMTick(VMState& state, const Program& program)
{
    if (state.halted)
        return {IntentType::None, 0};

    // Still busy from multi-tick instruction.
    if (state.busyLeft > 0)
    {
        --state.busyLeft;
        return {IntentType::None, 0};
    }

    // PC safety.
    if (program.code.empty() || state.pc >= static_cast<uint16_t>(program.code.size()))
    {
        state.halted = true;
        return {IntentType::None, 0};
    }

    const Instruction& instr = program.code[state.pc];
    Intent intent{IntentType::None, 0};

    // Advance PC (wraps).
    auto advancePC = [&]() {
        state.pc = static_cast<uint16_t>((state.pc + 1) % program.code.size());
    };

    switch (instr.opcode)
    {
    case Opcode::MOVE:
        intent = {IntentType::Move, 0};
        state.busyLeft = TickCost[static_cast<int>(Opcode::MOVE)] - 1;
        advancePC();
        break;

    case Opcode::TURN:
        intent = {IntentType::Turn, instr.imm16};
        state.busyLeft = TickCost[static_cast<int>(Opcode::TURN)] - 1;
        advancePC();
        break;

    case Opcode::SCAN:
        intent = {IntentType::Scan, 0};
        state.busyLeft = TickCost[static_cast<int>(Opcode::SCAN)] - 1;
        advancePC();
        break;

    case Opcode::FIRE:
        intent = {IntentType::Fire, 0};
        state.busyLeft = TickCost[static_cast<int>(Opcode::FIRE)] - 1;
        advancePC();
        break;

    case Opcode::SHIELD:
        intent = {IntentType::Shield, 0};
        state.busyLeft = TickCost[static_cast<int>(Opcode::SHIELD)] - 1;
        advancePC();
        break;

    case Opcode::WAIT:
        intent = {IntentType::Wait, 0};
        state.busyLeft = TickCost[static_cast<int>(Opcode::WAIT)] - 1;
        advancePC();
        break;

    case Opcode::SET:
    {
        uint8_t reg = instr.operandA;
        if (reg < GPRegisterCount)
            state.regs[reg] = static_cast<int32_t>(instr.imm16);
        state.busyLeft = TickCost[static_cast<int>(Opcode::SET)] - 1;
        advancePC();
        // SET is internal; emit Wait intent for tick cost.
        intent = {IntentType::Wait, 0};
        break;
    }

    case Opcode::IF:
    {
        int32_t lhs = state.regs[instr.operandA];
        int32_t rhs = (instr.reserved == 1)
            ? state.regs[static_cast<uint8_t>(instr.imm16)]
            : static_cast<int32_t>(instr.imm16);
        CmpOp cmp = static_cast<CmpOp>(instr.operandB);
        bool taken = false;
        switch (cmp)
        {
        case CmpOp::EQ: taken = (lhs == rhs); break;
        case CmpOp::NE: taken = (lhs != rhs); break;
        case CmpOp::LT: taken = (lhs <  rhs); break;
        case CmpOp::LE: taken = (lhs <= rhs); break;
        case CmpOp::GT: taken = (lhs >  rhs); break;
        case CmpOp::GE: taken = (lhs >= rhs); break;
        }
        if (taken)
        {
            state.pc = instr.target;
            // Wrap safety.
            if (state.pc >= static_cast<uint16_t>(program.code.size()))
                state.pc = static_cast<uint16_t>(state.pc % program.code.size());
        }
        else
        {
            advancePC();
        }
        state.busyLeft = TickCost[static_cast<int>(Opcode::IF)] - 1;
        intent = {IntentType::Wait, 0};
        break;
    }

    default:
        state.halted = true;
        break;
    }

    return intent;
}

} // namespace Automata
