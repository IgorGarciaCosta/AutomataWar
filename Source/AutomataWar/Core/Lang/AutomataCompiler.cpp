/**
 * @file AutomataCompiler.cpp
 * @brief Implementation of the Automata War assembly compiler.
 */

#include "AutomataCompiler.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>

namespace Automata
{

    namespace
    {

        int32_t Levenshtein(const std::string &a, const std::string &b)
        {
            const size_t m = a.size(), n = b.size();
            std::vector<int32_t> prev(n + 1), curr(n + 1);
            for (size_t j = 0; j <= n; ++j)
                prev[j] = static_cast<int32_t>(j);
            for (size_t i = 1; i <= m; ++i)
            {
                curr[0] = static_cast<int32_t>(i);
                for (size_t j = 1; j <= n; ++j)
                {
                    int32_t cost = (std::toupper(static_cast<unsigned char>(a[i - 1])) ==
                                    std::toupper(static_cast<unsigned char>(b[j - 1])))
                                       ? 0
                                       : 1;
                    curr[j] = std::min({prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
                }
                std::swap(prev, curr);
            }
            return prev[n];
        }

        std::string ToUpper(std::string s)
        {
            for (auto &c : s)
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return s;
        }

        int32_t ParseRegister(const std::string &tok)
        {
            static const std::unordered_map<std::string, int32_t> regMap = {
                {"R0", 0}, {"R1", 1}, {"R2", 2}, {"R3", 3}, {"R_HP", 4}, {"R_ENEMY_DIST", 5}, {"R_ENEMY_DIR", 6}, {"R_ENERGY", 7}, {"R_TICK", 8}};
            auto it = regMap.find(ToUpper(tok));
            return it != regMap.end() ? it->second : -1;
        }

        // Only accept symbolic operators, reject aliases EQ/NE/LT/GT/LE/GE and GOTO.
        int32_t ParseCmpOp(const std::string &tok)
        {
            if (tok == "==")
                return static_cast<int32_t>(CmpOp::EQ);
            if (tok == "!=")
                return static_cast<int32_t>(CmpOp::NE);
            if (tok == "<")
                return static_cast<int32_t>(CmpOp::LT);
            if (tok == "<=")
                return static_cast<int32_t>(CmpOp::LE);
            if (tok == ">")
                return static_cast<int32_t>(CmpOp::GT);
            if (tok == ">=")
                return static_cast<int32_t>(CmpOp::GE);
            return -1;
        }

        bool IsRejectedAlias(const std::string &tok)
        {
            std::string u = ToUpper(tok);
            return u == "EQ" || u == "NE" || u == "LT" || u == "LE" || u == "GT" || u == "GE" || u == "GOTO";
        }

        bool ParseImmediate(const std::string &tok, int32_t &out)
        {
            if (tok.empty())
                return false;
            try
            {
                size_t pos = 0;
                long long val = std::stoll(tok, &pos);
                if (pos != tok.size())
                    return false;
                out = static_cast<int32_t>(val);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        std::vector<std::string> Tokenize(const std::string &line)
        {
            std::vector<std::string> tokens;
            size_t i = 0;
            const size_t len = line.size();
            while (i < len)
            {
                while (i < len && (line[i] == ' ' || line[i] == '\t' || line[i] == ','))
                    ++i;
                if (i >= len)
                    break;
                if (line[i] == ';')
                    break;
                if (i + 1 < len && line[i] == '/' && line[i + 1] == '/')
                    break;
                size_t start = i;
                if (i + 1 < len && ((line[i] == '!' || line[i] == '<' || line[i] == '>' || line[i] == '=') && line[i + 1] == '='))
                {
                    tokens.push_back(line.substr(start, 2));
                    i += 2;
                }
                else if (line[i] == '<' || line[i] == '>')
                {
                    tokens.push_back(line.substr(start, 1));
                    i += 1;
                }
                else
                {
                    while (i < len && line[i] != ' ' && line[i] != '\t' && line[i] != ',' && line[i] != ';')
                    {
                        if (i + 1 < len && line[i] == '/' && line[i + 1] == '/')
                            break;
                        ++i;
                    }
                    if (i > start)
                        tokens.push_back(line.substr(start, i - start));
                }
            }
            return tokens;
        }

        int32_t FindColumn(const std::string &line, const std::string &tok, int32_t hint)
        {
            auto pos = line.find(tok, static_cast<size_t>(hint > 0 ? hint - 1 : 0));
            return pos != std::string::npos ? static_cast<int32_t>(pos) + 1 : 1;
        }

    } // anonymous namespace

    bool CompileResult::Ok() const
    {
        for (auto &d : diagnostics)
            if (d.severity == DiagSeverity::Error)
                return false;
        return true;
    }

    CompileResult Compile(const std::string &source)
    {
        CompileResult result;
        auto &diags = result.diagnostics;

        std::vector<std::string> lines;
        {
            std::istringstream stream(source);
            std::string ln;
            while (std::getline(stream, ln))
                lines.push_back(ln);
        }

        if (static_cast<int32_t>(lines.size()) > MaxSourceLines)
        {
            diags.push_back({DiagSeverity::Error, DiagKind::SourceTooLong, 1, 1,
                             "Source exceeds maximum of " + std::to_string(MaxSourceLines) + " lines.", ""});
            return result;
        }

        struct RawLine
        {
            int32_t lineNum;
            std::vector<std::string> tokens;
        };
        std::vector<RawLine> rawLines;
        std::unordered_map<std::string, int32_t> labelMap;

        for (int32_t lineIdx = 0; lineIdx < static_cast<int32_t>(lines.size()); ++lineIdx)
        {
            auto tokens = Tokenize(lines[lineIdx]);
            if (tokens.empty())
                continue;

            while (!tokens.empty() && tokens[0].size() > 1 && tokens[0].back() == ':')
            {
                std::string label = ToUpper(tokens[0].substr(0, tokens[0].size() - 1));
                int32_t instrIdx = static_cast<int32_t>(rawLines.size());
                if (labelMap.count(label))
                {
                    diags.push_back({DiagSeverity::Error, DiagKind::DuplicateLabel, lineIdx + 1,
                                     FindColumn(lines[lineIdx], tokens[0], 0),
                                     "Duplicate label '" + label + "'.", ""});
                }
                else
                {
                    labelMap[label] = instrIdx;
                }
                tokens.erase(tokens.begin());
            }
            if (!tokens.empty())
                rawLines.push_back({lineIdx + 1, std::move(tokens)});
        }

        if (static_cast<int32_t>(rawLines.size()) > MaxProgramLength)
        {
            diags.push_back({DiagSeverity::Error, DiagKind::ProgramTooLong, 1, 1,
                             "Program exceeds maximum of " + std::to_string(MaxProgramLength) + " instructions.", ""});
            return result;
        }

        std::vector<Instruction> code;
        std::vector<SourceLocation> srcMap;
        code.reserve(rawLines.size());
        srcMap.reserve(rawLines.size());

        struct LabelRef
        {
            size_t instrIdx;
            std::string label;
            int32_t line;
            int32_t col;
        };
        std::vector<LabelRef> labelRefs;

        static const std::array<std::string, OpcodeCount> Mnemonics = {
            "MOVE", "TURN", "SCAN", "FIRE", "SHIELD", "SET", "IF", "WAIT"};

        for (auto &raw : rawLines)
        {
            const auto &tokens = raw.tokens;
            const std::string &origLine = lines[raw.lineNum - 1];
            std::string mnem = ToUpper(tokens[0]);

            int32_t opcodeIdx = -1;
            for (int32_t i = 0; i < OpcodeCount; ++i)
                if (Mnemonics[i] == mnem)
                {
                    opcodeIdx = i;
                    break;
                }

            if (opcodeIdx < 0)
            {
                std::string suggestion;
                int32_t bestDist = 999;
                for (auto &m : Mnemonics)
                {
                    int32_t d = Levenshtein(mnem, m);
                    if (d < bestDist)
                    {
                        bestDist = d;
                        suggestion = m;
                    }
                }
                std::string sugText = (bestDist <= 3) ? suggestion : "";
                std::string msg = "Unknown instruction '" + tokens[0] + "'.";
                if (!sugText.empty())
                    msg += " Did you mean '" + sugText + "'?";
                diags.push_back({DiagSeverity::Error, DiagKind::UnknownInstruction, raw.lineNum,
                                 FindColumn(origLine, tokens[0], 0), msg, sugText});
                code.push_back({});
                srcMap.push_back({raw.lineNum, FindColumn(origLine, tokens[0], 0)});
                continue;
            }

            Opcode op = static_cast<Opcode>(opcodeIdx);
            Instruction instr;
            instr.opcode = op;

            switch (op)
            {
            case Opcode::MOVE:
            {
                if (static_cast<int32_t>(tokens.size()) - 1 != 1)
                {
                    diags.push_back({DiagSeverity::Error, DiagKind::BadOperandCount, raw.lineNum, 1,
                                     "MOVE takes 1 operand (FWD or BACK), got " + std::to_string(tokens.size() - 1) + ".", ""});
                }
                else
                {
                    std::string dir = ToUpper(tokens[1]);
                    if (dir == "FWD")
                        instr.operandA = static_cast<uint8_t>(MoveDir::Forward);
                    else if (dir == "BACK")
                        instr.operandA = static_cast<uint8_t>(MoveDir::Backward);
                    else
                    {
                        diags.push_back({DiagSeverity::Error, DiagKind::BadOperandType, raw.lineNum,
                                         FindColumn(origLine, tokens[1], 0),
                                         "MOVE operand must be FWD or BACK, got '" + tokens[1] + "'.", ""});
                    }
                }
                break;
            }

            case Opcode::TURN:
            {
                if (static_cast<int32_t>(tokens.size()) - 1 != 1)
                {
                    diags.push_back({DiagSeverity::Error, DiagKind::BadOperandCount, raw.lineNum, 1,
                                     "TURN takes 1 operand (LEFT or RIGHT), got " + std::to_string(tokens.size() - 1) + ".", ""});
                }
                else
                {
                    std::string dir = ToUpper(tokens[1]);
                    if (dir == "LEFT")
                        instr.imm16 = -1;
                    else if (dir == "RIGHT")
                        instr.imm16 = 1;
                    else
                    {
                        diags.push_back({DiagSeverity::Error, DiagKind::BadOperandType, raw.lineNum,
                                         FindColumn(origLine, tokens[1], 0),
                                         "TURN operand must be LEFT or RIGHT, got '" + tokens[1] + "'.", ""});
                    }
                }
                break;
            }

            case Opcode::SCAN:
            case Opcode::FIRE:
            case Opcode::SHIELD:
            case Opcode::WAIT:
                if (static_cast<int32_t>(tokens.size()) - 1 != 0)
                {
                    diags.push_back({DiagSeverity::Error, DiagKind::BadOperandCount, raw.lineNum, 1,
                                     mnem + " takes 0 operands, got " + std::to_string(tokens.size() - 1) + ".", ""});
                }
                break;

            case Opcode::SET:
            {
                if (static_cast<int32_t>(tokens.size()) - 1 != 2)
                {
                    diags.push_back({DiagSeverity::Error, DiagKind::BadOperandCount, raw.lineNum, 1,
                                     "SET takes 2 operands (Rn imm), got " + std::to_string(tokens.size() - 1) + ".", ""});
                }
                else
                {
                    int32_t reg = ParseRegister(tokens[1]);
                    if (reg < 0)
                    {
                        diags.push_back({DiagSeverity::Error, DiagKind::BadOperandType, raw.lineNum,
                                         FindColumn(origLine, tokens[1], 0),
                                         "Expected register name, got '" + tokens[1] + "'.", ""});
                    }
                    else if (reg >= FirstSystemReg)
                    {
                        diags.push_back({DiagSeverity::Error, DiagKind::SetToReadOnly, raw.lineNum,
                                         FindColumn(origLine, tokens[1], 0),
                                         "Cannot SET system read-only register '" + tokens[1] + "'.", ""});
                    }
                    else
                    {
                        instr.operandA = static_cast<uint8_t>(reg);
                    }

                    int32_t imm = 0;
                    if (!ParseImmediate(tokens[2], imm))
                    {
                        diags.push_back({DiagSeverity::Error, DiagKind::BadOperandType, raw.lineNum,
                                         FindColumn(origLine, tokens[2], 0),
                                         "Expected integer immediate, got '" + tokens[2] + "'.", ""});
                    }
                    else if (imm < ImmMin || imm > ImmMax)
                    {
                        diags.push_back({DiagSeverity::Error, DiagKind::ImmediateOutOfRange, raw.lineNum,
                                         FindColumn(origLine, tokens[2], 0),
                                         "Immediate " + std::to_string(imm) + " outside [-32768, 32767].", ""});
                    }
                    else
                    {
                        instr.imm16 = static_cast<int16_t>(imm);
                    }
                }
                break;
            }

            case Opcode::IF:
            {
                // IF reg op imm JUMP label = 6 tokens total (IF + 5 operands)
                if (static_cast<int32_t>(tokens.size()) - 1 != 5)
                {
                    diags.push_back({DiagSeverity::Error, DiagKind::BadOperandCount, raw.lineNum, 1,
                                     "IF requires exactly: IF <reg> <op> <imm> JUMP <label> (6 tokens), got " +
                                         std::to_string(tokens.size()) + ".",
                                     ""});
                }
                else
                {
                    // tokens[1] = reg
                    int32_t regA = ParseRegister(tokens[1]);
                    if (regA < 0)
                    {
                        diags.push_back({DiagSeverity::Error, DiagKind::BadOperandType, raw.lineNum,
                                         FindColumn(origLine, tokens[1], 0),
                                         "Expected register, got '" + tokens[1] + "'.", ""});
                    }
                    else
                    {
                        instr.operandA = static_cast<uint8_t>(regA);
                    }

                    // tokens[2] = comparison operator
                    if (IsRejectedAlias(tokens[2]))
                    {
                        diags.push_back({DiagSeverity::Error, DiagKind::AliasRejected, raw.lineNum,
                                         FindColumn(origLine, tokens[2], 0),
                                         "Alias '" + tokens[2] + "' rejected. Use symbolic operators: == != < > <= >=.", ""});
                    }
                    else
                    {
                        int32_t cmpVal = ParseCmpOp(tokens[2]);
                        if (cmpVal < 0)
                        {
                            diags.push_back({DiagSeverity::Error, DiagKind::MalformedComparison, raw.lineNum,
                                             FindColumn(origLine, tokens[2], 0),
                                             "Unknown comparison operator '" + tokens[2] + "'. Use == != < > <= >=.", ""});
                        }
                        else
                        {
                            instr.operandB = static_cast<uint8_t>(cmpVal);
                        }
                    }

                    // tokens[3] = immediate (right operand must be immediate, not register)
                    int32_t imm = 0;
                    if (ParseRegister(tokens[3]) >= 0)
                    {
                        diags.push_back({DiagSeverity::Error, DiagKind::BadOperandType, raw.lineNum,
                                         FindColumn(origLine, tokens[3], 0),
                                         "IF right operand must be an immediate value, not a register. Got '" + tokens[3] + "'.", ""});
                    }
                    else if (!ParseImmediate(tokens[3], imm))
                    {
                        diags.push_back({DiagSeverity::Error, DiagKind::BadOperandType, raw.lineNum,
                                         FindColumn(origLine, tokens[3], 0),
                                         "Expected immediate value, got '" + tokens[3] + "'.", ""});
                    }
                    else if (imm < ImmMin || imm > ImmMax)
                    {
                        diags.push_back({DiagSeverity::Error, DiagKind::ImmediateOutOfRange, raw.lineNum,
                                         FindColumn(origLine, tokens[3], 0),
                                         "Immediate " + std::to_string(imm) + " outside [-32768, 32767].", ""});
                    }
                    else
                    {
                        instr.imm16 = static_cast<int16_t>(imm);
                    }

                    // tokens[4] = must be literal "JUMP"
                    if (ToUpper(tokens[4]) != "JUMP")
                    {
                        if (ToUpper(tokens[4]) == "GOTO")
                        {
                            diags.push_back({DiagSeverity::Error, DiagKind::AliasRejected, raw.lineNum,
                                             FindColumn(origLine, tokens[4], 0),
                                             "GOTO rejected. Use JUMP.", ""});
                        }
                        else
                        {
                            diags.push_back({DiagSeverity::Error, DiagKind::MissingJumpKeyword, raw.lineNum,
                                             FindColumn(origLine, tokens[4], 0),
                                             "Expected JUMP keyword, got '" + tokens[4] + "'.", ""});
                        }
                    }

                    // tokens[5] = label (resolve later)
                    std::string label = ToUpper(tokens[5]);
                    labelRefs.push_back({code.size(), label, raw.lineNum, FindColumn(origLine, tokens[5], 0)});
                }
                break;
            }

            default:
                break;
            }

            code.push_back(instr);
            srcMap.push_back({raw.lineNum, FindColumn(origLine, tokens[0], 0)});
        }

        // Resolve label references.
        for (auto &ref : labelRefs)
        {
            auto it = labelMap.find(ref.label);
            if (it == labelMap.end())
            {
                diags.push_back({DiagSeverity::Error, DiagKind::UnknownLabel, ref.line, ref.col,
                                 "Unknown label '" + ref.label + "'.", ""});
            }
            else
            {
                code[ref.instrIdx].target = static_cast<uint16_t>(it->second);
            }
        }

        if (result.Ok())
        {
            result.program.code = std::move(code);
            result.program.sourceMap = std::move(srcMap);
        }

        return result;
    }

} // namespace Automata
