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

// ─── Helpers ─────────────────────────────────────────────────────────────────

namespace
{

/** Levenshtein distance between two short strings. */
int32_t Levenshtein(const std::string& a, const std::string& b)
{
    const size_t m = a.size(), n = b.size();
    std::vector<int32_t> prev(n + 1), curr(n + 1);
    for (size_t j = 0; j <= n; ++j) prev[j] = static_cast<int32_t>(j);
    for (size_t i = 1; i <= m; ++i)
    {
        curr[0] = static_cast<int32_t>(i);
        for (size_t j = 1; j <= n; ++j)
        {
            int32_t cost = (std::toupper(static_cast<unsigned char>(a[i-1])) ==
                            std::toupper(static_cast<unsigned char>(b[j-1]))) ? 0 : 1;
            curr[j] = std::min({prev[j] + 1, curr[j-1] + 1, prev[j-1] + cost});
        }
        std::swap(prev, curr);
    }
    return prev[n];
}

/** Uppercase a string in place. */
std::string ToUpper(std::string s)
{
    for (auto& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

/** Known instruction mnemonics. */
static const std::array<std::string, OpcodeCount> Mnemonics = {
    "MOVE", "TURN", "SCAN", "FIRE", "SHIELD", "SET", "IF", "WAIT"
};

/** Try parse a register name, returns index or -1. */
int32_t ParseRegister(const std::string& tok)
{
    static const std::unordered_map<std::string, int32_t> regMap = {
        {"R0", 0}, {"R1", 1}, {"R2", 2}, {"R3", 3},
        {"R_HP", 4}, {"R_ENEMY_DIST", 5}, {"R_ENEMY_DIR", 6}, {"R_ENERGY", 7}, {"R_TICK", 8}
    };
    auto it = regMap.find(ToUpper(tok));
    return it != regMap.end() ? it->second : -1;
}

/** Try parse a comparison operator string, returns CmpOp or -1. */
int32_t ParseCmpOp(const std::string& tok)
{
    if (tok == "==" || tok == "EQ") return static_cast<int32_t>(CmpOp::EQ);
    if (tok == "!=" || tok == "NE") return static_cast<int32_t>(CmpOp::NE);
    if (tok == "<"  || tok == "LT") return static_cast<int32_t>(CmpOp::LT);
    if (tok == "<=" || tok == "LE") return static_cast<int32_t>(CmpOp::LE);
    if (tok == ">"  || tok == "GT") return static_cast<int32_t>(CmpOp::GT);
    if (tok == ">=" || tok == "GE") return static_cast<int32_t>(CmpOp::GE);
    return -1;
}

/** Try parse an integer immediate. */
bool ParseImmediate(const std::string& tok, int32_t& out)
{
    if (tok.empty()) return false;
    try
    {
        size_t pos = 0;
        long long val = std::stoll(tok, &pos);
        if (pos != tok.size()) return false;
        out = static_cast<int32_t>(val);
        return true;
    }
    catch (...) { return false; }
}

/** Tokenize a line into whitespace/comma separated tokens, stripping comments. */
std::vector<std::string> Tokenize(const std::string& line)
{
    std::vector<std::string> tokens;
    size_t i = 0;
    const size_t len = line.size();
    while (i < len)
    {
        // skip whitespace/commas
        while (i < len && (line[i] == ' ' || line[i] == '\t' || line[i] == ',')) ++i;
        if (i >= len) break;
        // comment
        if (line[i] == ';') break;
        if (i + 1 < len && line[i] == '/' && line[i+1] == '/') break;
        // collect token (operators stick together: ==, !=, <=, >=)
        size_t start = i;
        if (i + 1 < len && ((line[i] == '!' || line[i] == '<' || line[i] == '>' || line[i] == '=') && line[i+1] == '='))
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
                if (i + 1 < len && line[i] == '/' && line[i+1] == '/') break;
                ++i;
            }
            if (i > start) tokens.push_back(line.substr(start, i - start));
        }
    }
    return tokens;
}

/** Find column (1-based) of a token in original line. */
int32_t FindColumn(const std::string& line, const std::string& tok, int32_t hint)
{
    auto pos = line.find(tok, static_cast<size_t>(hint > 0 ? hint - 1 : 0));
    return pos != std::string::npos ? static_cast<int32_t>(pos) + 1 : 1;
}

} // anonymous namespace

// ─── CompileResult ───────────────────────────────────────────────────────────

bool CompileResult::Ok() const
{
    for (auto& d : diagnostics)
        if (d.severity == DiagSeverity::Error) return false;
    return true;
}

// ─── Compile ─────────────────────────────────────────────────────────────────

CompileResult Compile(const std::string& source)
{
    CompileResult result;
    auto& diags = result.diagnostics;

    // Split into lines.
    std::vector<std::string> lines;
    {
        std::istringstream stream(source);
        std::string ln;
        while (std::getline(stream, ln)) lines.push_back(ln);
    }

    if (static_cast<int32_t>(lines.size()) > MaxSourceLines)
    {
        diags.push_back({DiagSeverity::Error, DiagKind::SourceTooLong, 1, 1,
                         "Source exceeds maximum of " + std::to_string(MaxSourceLines) + " lines.", ""});
        return result;
    }

    // First pass: collect labels and raw token lines.
    struct RawLine { int32_t lineNum; std::vector<std::string> tokens; };
    std::vector<RawLine> rawLines;
    std::unordered_map<std::string, int32_t> labelMap; // label -> instruction index

    for (int32_t lineIdx = 0; lineIdx < static_cast<int32_t>(lines.size()); ++lineIdx)
    {
        auto tokens = Tokenize(lines[lineIdx]);
        if (tokens.empty()) continue;

        // Label detection: token ending with ':'
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

    // Second pass: encode instructions.
    std::vector<Instruction> code;
    code.reserve(rawLines.size());

    // Track labels that need resolving.
    struct LabelRef { size_t instrIdx; std::string label; int32_t line; int32_t col; };
    std::vector<LabelRef> labelRefs;

    for (auto& raw : rawLines)
    {
        const auto& tokens = raw.tokens;
        const std::string& origLine = lines[raw.lineNum - 1];
        std::string mnem = ToUpper(tokens[0]);

        // Find opcode.
        int32_t opcodeIdx = -1;
        for (int32_t i = 0; i < OpcodeCount; ++i)
            if (Mnemonics[i] == mnem) { opcodeIdx = i; break; }

        if (opcodeIdx < 0)
        {
            // Suggest closest mnemonic.
            std::string suggestion;
            int32_t bestDist = 999;
            for (auto& m : Mnemonics)
            {
                int32_t d = Levenshtein(mnem, m);
                if (d < bestDist) { bestDist = d; suggestion = m; }
            }
            std::string sugText = (bestDist <= 3) ? suggestion : "";
            std::string msg = "Unknown instruction '" + tokens[0] + "'.";
            if (!sugText.empty()) msg += " Did you mean '" + sugText + "'?";
            diags.push_back({DiagSeverity::Error, DiagKind::UnknownInstruction, raw.lineNum,
                             FindColumn(origLine, tokens[0], 0), msg, sugText});
            code.push_back({}); // placeholder
            continue;
        }

        Opcode op = static_cast<Opcode>(opcodeIdx);
        Instruction instr;
        instr.opcode = op;
        int32_t expectedOperands = 0;

        switch (op)
        {
        case Opcode::MOVE:
        case Opcode::SCAN:
        case Opcode::FIRE:
        case Opcode::SHIELD:
        case Opcode::WAIT:
            expectedOperands = 0;
            if (static_cast<int32_t>(tokens.size()) - 1 != 0)
            {
                diags.push_back({DiagSeverity::Error, DiagKind::BadOperandCount, raw.lineNum, 1,
                                 mnem + " takes 0 operands, got " + std::to_string(tokens.size()-1) + ".", ""});
            }
            break;

        case Opcode::TURN:
        {
            expectedOperands = 1;
            if (static_cast<int32_t>(tokens.size()) - 1 != 1)
            {
                diags.push_back({DiagSeverity::Error, DiagKind::BadOperandCount, raw.lineNum, 1,
                                 "TURN takes 1 operand (-1 or 1), got " + std::to_string(tokens.size()-1) + ".", ""});
            }
            else
            {
                int32_t val = 0;
                if (!ParseImmediate(tokens[1], val) || (val != -1 && val != 1))
                {
                    diags.push_back({DiagSeverity::Error, DiagKind::BadOperandType, raw.lineNum,
                                     FindColumn(origLine, tokens[1], 0),
                                     "TURN operand must be -1 (left) or 1 (right).", ""});
                }
                else
                {
                    instr.imm16 = static_cast<int16_t>(val);
                }
            }
            break;
        }

        case Opcode::SET:
        {
            expectedOperands = 2;
            if (static_cast<int32_t>(tokens.size()) - 1 != 2)
            {
                diags.push_back({DiagSeverity::Error, DiagKind::BadOperandCount, raw.lineNum, 1,
                                 "SET takes 2 operands (Rx, imm16), got " + std::to_string(tokens.size()-1) + ".", ""});
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
            // IF Rx cmp Ry/imm label
            expectedOperands = 4;
            if (static_cast<int32_t>(tokens.size()) - 1 < 4)
            {
                diags.push_back({DiagSeverity::Error, DiagKind::BadOperandCount, raw.lineNum, 1,
                                 "IF takes 4 operands (Rx cmp Ry/imm label), got " + std::to_string(tokens.size()-1) + ".", ""});
            }
            else
            {
                // Rx
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

                // cmp
                int32_t cmpVal = ParseCmpOp(tokens[2]);
                if (cmpVal < 0)
                {
                    diags.push_back({DiagSeverity::Error, DiagKind::MalformedComparison, raw.lineNum,
                                     FindColumn(origLine, tokens[2], 0),
                                     "Malformed comparison operator '" + tokens[2] + "'. Use ==, !=, <, <=, >, >=.", ""});
                }
                else
                {
                    instr.operandB = static_cast<uint8_t>(cmpVal);
                }

                // Ry or imm
                int32_t regB = ParseRegister(tokens[3]);
                if (regB >= 0)
                {
                    instr.reserved = 1; // flag: operandC is register
                    instr.imm16 = static_cast<int16_t>(regB);
                }
                else
                {
                    int32_t imm = 0;
                    if (!ParseImmediate(tokens[3], imm))
                    {
                        diags.push_back({DiagSeverity::Error, DiagKind::BadOperandType, raw.lineNum,
                                         FindColumn(origLine, tokens[3], 0),
                                         "Expected register or immediate, got '" + tokens[3] + "'.", ""});
                    }
                    else if (imm < ImmMin || imm > ImmMax)
                    {
                        diags.push_back({DiagSeverity::Error, DiagKind::ImmediateOutOfRange, raw.lineNum,
                                         FindColumn(origLine, tokens[3], 0),
                                         "Immediate " + std::to_string(imm) + " outside [-32768, 32767].", ""});
                    }
                    else
                    {
                        instr.reserved = 0; // flag: operandC is immediate
                        instr.imm16 = static_cast<int16_t>(imm);
                    }
                }

                // label (resolve later)
                std::string label = ToUpper(tokens[4]);
                labelRefs.push_back({code.size(), label, raw.lineNum, FindColumn(origLine, tokens[4], 0)});
            }
            break;
        }

        default:
            break;
        }

        code.push_back(instr);
    }

    // Resolve label references.
    for (auto& ref : labelRefs)
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
        result.program.code = std::move(code);

    return result;
}

} // namespace Automata
