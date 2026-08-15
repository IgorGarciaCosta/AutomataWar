/**
 * @file AutomataReplay.cpp
 * @brief Implementation of the Automata War replay codec.
 */

#include "AutomataReplay.h"
#include <cstring>

namespace Automata
{

    namespace
    {

        /** CRC-32 (ISO 3720 / zlib polynomial). */
        uint32_t Crc32(const uint8_t *data, size_t len)
        {
            uint32_t crc = 0xFFFFFFFF;
            for (size_t i = 0; i < len; ++i)
            {
                crc ^= data[i];
                for (int b = 0; b < 8; ++b)
                    crc = (crc >> 1) ^ (0xEDB88320 & (~(crc & 1) + 1));
            }
            return ~crc;
        }

        void WriteU16(std::vector<uint8_t> &out, uint16_t v)
        {
            out.push_back(static_cast<uint8_t>(v & 0xFF));
            out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        }

        void WriteU32(std::vector<uint8_t> &out, uint32_t v)
        {
            for (int i = 0; i < 4; ++i)
                out.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
        }

        void WriteU64(std::vector<uint8_t> &out, uint64_t v)
        {
            for (int i = 0; i < 8; ++i)
                out.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
        }

        uint16_t ReadU16(const uint8_t *p) { return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8); }
        uint32_t ReadU32(const uint8_t *p)
        {
            uint32_t v = 0;
            for (int i = 0; i < 4; ++i)
                v |= static_cast<uint32_t>(p[i]) << (i * 8);
            return v;
        }
        uint64_t ReadU64(const uint8_t *p)
        {
            uint64_t v = 0;
            for (int i = 0; i < 8; ++i)
                v |= static_cast<uint64_t>(p[i]) << (i * 8);
            return v;
        }

        /** Encode effect flags and bounded round counters into four bytes. */
        void WriteEffects(std::vector<uint8_t> &out, const FAWRobotEffects &Effects)
        {
            out.push_back((Effects.bShieldCharged ? 1 : 0) | (Effects.bAccelerateNextMove ? 2 : 0));
            out.push_back(static_cast<uint8_t>(FMath::Clamp(Effects.ExtraAmmoRounds, 0, 255)));
            out.push_back(static_cast<uint8_t>(FMath::Clamp(Effects.ShieldRounds, 0, 255)));
            out.push_back(static_cast<uint8_t>(FMath::Clamp(Effects.AcceleratorRounds, 0, 255)));
        }

        /** Decode the four-byte persistent-effect block from a validated replay buffer. */
        FAWRobotEffects ReadEffects(const uint8_t *p)
        {
            FAWRobotEffects Effects;
            Effects.bShieldCharged = (p[0] & 1) != 0;
            Effects.bAccelerateNextMove = (p[0] & 2) != 0;
            Effects.ExtraAmmoRounds = p[1];
            Effects.ShieldRounds = p[2];
            Effects.AcceleratorRounds = p[3];
            return Effects;
        }

        static constexpr char B64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        int B64Decode(char c)
        {
            if (c >= 'A' && c <= 'Z')
                return c - 'A';
            if (c >= 'a' && c <= 'z')
                return c - 'a' + 26;
            if (c >= '0' && c <= '9')
                return c - '0' + 52;
            if (c == '+')
                return 62;
            if (c == '/')
                return 63;
            return -1;
        }

    } // anonymous namespace

    // ─── Encode ──────────────────────────────────────────────────────────────────

    std::vector<uint8_t> EncodeReplay(const ReplayData &data)
    {
        std::vector<uint8_t> out;
        out.reserve(32 + data.commandsA.Num() + data.commandsB.Num());

        // Magic.
        out.push_back('A');
        out.push_back('W');
        out.push_back('R');
        out.push_back('P');
        WriteU16(out, data.version);
        WriteU64(out, data.rulesetHash);
        WriteU64(out, data.seed);
        WriteU32(out, static_cast<uint32_t>(FMath::Max(0, data.initialActionPointsA)));
        WriteU32(out, static_cast<uint32_t>(FMath::Max(0, data.initialActionPointsB)));
        WriteEffects(out, data.initialEffectsA);
        WriteEffects(out, data.initialEffectsB);

        // Commands A.
        uint16_t lenA = static_cast<uint16_t>(FMath::Min<int32>(data.commandsA.Num(), MaxCommands));
        WriteU16(out, lenA);
        for (uint16_t Index = 0; Index < lenA; ++Index)
            out.push_back(static_cast<uint8_t>(data.commandsA[Index]));

        // Commands B.
        uint16_t lenB = static_cast<uint16_t>(FMath::Min<int32>(data.commandsB.Num(), MaxCommands));
        WriteU16(out, lenB);
        for (uint16_t Index = 0; Index < lenB; ++Index)
            out.push_back(static_cast<uint8_t>(data.commandsB[Index]));

        // CRC.
        uint32_t crc = Crc32(out.data(), out.size());
        WriteU32(out, crc);

        return out;
    }

    // ─── Decode ──────────────────────────────────────────────────────────────────

    ReplayDecodeResult DecodeReplay(const std::vector<uint8_t> &bytes)
    {
        ReplayDecodeResult result;
        const size_t minSize = 4 + 2 + 8 + 8 + 4 + 4 + 4 + 4 + 2 + 2 + 4; // 46 bytes minimum

        if (bytes.size() < minSize)
        {
            result.error = ReplayError::Truncated;
            return result;
        }

        const uint8_t *p = bytes.data();

        // Magic.
        if (p[0] != 'A' || p[1] != 'W' || p[2] != 'R' || p[3] != 'P')
        {
            result.error = ReplayError::InvalidMagic;
            return result;
        }
        p += 4;

        result.data.version = ReadU16(p);
        p += 2;
        if (result.data.version != ReplayVersion)
        {
            result.error = ReplayError::VersionMismatch;
            return result;
        }

        result.data.rulesetHash = ReadU64(p);
        p += 8;
        if (result.data.rulesetHash != RulesetHash)
        {
            result.error = ReplayError::RulesetMismatch;
            return result;
        }

        result.data.seed = ReadU64(p);
        p += 8;

        result.data.initialActionPointsA = static_cast<int32_t>(ReadU32(p));
        p += 4;
        result.data.initialActionPointsB = static_cast<int32_t>(ReadU32(p));
        p += 4;
        result.data.initialEffectsA = ReadEffects(p);
        p += 4;
        result.data.initialEffectsB = ReadEffects(p);
        p += 4;

        // Commands A.
        size_t remaining = bytes.size() - static_cast<size_t>(p - bytes.data());
        if (remaining < 2)
        {
            result.error = ReplayError::Truncated;
            return result;
        }
        uint16_t lenA = ReadU16(p);
        p += 2;
        remaining -= 2;
        if (lenA == 0 || lenA > MaxCommands)
        {
            result.error = ReplayError::InvalidCommands;
            return result;
        }
        if (remaining < lenA)
        {
            result.error = ReplayError::Truncated;
            return result;
        }
        result.data.commandsA.Reserve(lenA);
        for (uint16_t Index = 0; Index < lenA; ++Index)
        {
            if (p[Index] >= static_cast<uint8_t>(EAWCommand::Count))
            {
                result.error = ReplayError::InvalidCommands;
                return result;
            }
            result.data.commandsA.Add(static_cast<EAWCommand>(p[Index]));
        }
        p += lenA;
        remaining -= lenA;

        // Commands B.
        if (remaining < 2)
        {
            result.error = ReplayError::Truncated;
            return result;
        }
        uint16_t lenB = ReadU16(p);
        p += 2;
        remaining -= 2;
        if (lenB == 0 || lenB > MaxCommands)
        {
            result.error = ReplayError::InvalidCommands;
            return result;
        }
        if (remaining < lenB)
        {
            result.error = ReplayError::Truncated;
            return result;
        }
        result.data.commandsB.Reserve(lenB);
        for (uint16_t Index = 0; Index < lenB; ++Index)
        {
            if (p[Index] >= static_cast<uint8_t>(EAWCommand::Count))
            {
                result.error = ReplayError::InvalidCommands;
                return result;
            }
            result.data.commandsB.Add(static_cast<EAWCommand>(p[Index]));
        }
        p += lenB;
        remaining -= lenB;

        // CRC.
        if (remaining < 4)
        {
            result.error = ReplayError::Truncated;
            return result;
        }
        uint32_t storedCrc = ReadU32(p);
        size_t payloadLen = static_cast<size_t>(p - bytes.data());
        uint32_t computedCrc = Crc32(bytes.data(), payloadLen);
        if (storedCrc != computedCrc)
        {
            result.error = ReplayError::ChecksumFailed;
            return result;
        }

        return result;
    }

    // ─── Base64 ──────────────────────────────────────────────────────────────────

    std::string ReplayToBase64(const std::vector<uint8_t> &bytes)
    {
        std::string out;
        size_t i = 0;
        size_t len = bytes.size();
        out.reserve(((len + 2) / 3) * 4);

        while (i < len)
        {
            uint32_t a = bytes[i++];
            uint32_t b = (i < len) ? bytes[i++] : 0;
            uint32_t c = (i < len) ? bytes[i++] : 0;
            uint32_t triple = (a << 16) | (b << 8) | c;

            out.push_back(B64Chars[(triple >> 18) & 0x3F]);
            out.push_back(B64Chars[(triple >> 12) & 0x3F]);
            out.push_back((i > len + 1) ? '=' : B64Chars[(triple >> 6) & 0x3F]);
            out.push_back((i > len) ? '=' : B64Chars[triple & 0x3F]);
        }
        // Fix padding.
        size_t pad = (3 - (len % 3)) % 3;
        for (size_t p = 0; p < pad; ++p)
            out[out.size() - 1 - p] = '=';

        return out;
    }

    bool ReplayFromBase64(const std::string &b64, std::vector<uint8_t> &outBytes)
    {
        outBytes.clear();
        if (b64.empty())
            return true;

        outBytes.reserve((b64.size() / 4) * 3);

        size_t i = 0;
        size_t len = b64.size();
        while (i < len)
        {
            // Skip whitespace.
            while (i < len && (b64[i] == '\n' || b64[i] == '\r' || b64[i] == ' '))
                ++i;
            if (i >= len)
                break;

            int vals[4] = {0, 0, 0, 0};
            int pad = 0;
            for (int j = 0; j < 4 && i < len; ++j, ++i)
            {
                if (b64[i] == '=')
                {
                    pad++;
                    vals[j] = 0;
                }
                else
                {
                    int v = B64Decode(b64[i]);
                    if (v < 0)
                        return false;
                    vals[j] = v;
                }
            }
            uint32_t triple = (static_cast<uint32_t>(vals[0]) << 18) |
                              (static_cast<uint32_t>(vals[1]) << 12) |
                              (static_cast<uint32_t>(vals[2]) << 6) |
                              static_cast<uint32_t>(vals[3]);
            outBytes.push_back(static_cast<uint8_t>((triple >> 16) & 0xFF));
            if (pad < 2)
                outBytes.push_back(static_cast<uint8_t>((triple >> 8) & 0xFF));
            if (pad < 1)
                outBytes.push_back(static_cast<uint8_t>(triple & 0xFF));
        }
        return true;
    }

} // namespace Automata
