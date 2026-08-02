#pragma once

/**
 * @file AutomataReplay.h
 * @brief Compact binary replay codec for Automata War matches.
 *
 * A replay contains both script sources, seed, version, and ruleset hash.
 * No simulation snapshots are stored; the match is re-simulated from the replay.
 * Supports base64 import/export. Engine-independent.
 */

#include "AutomataWar/Core/AutomataRules.h"
#include <string>
#include <vector>
#include <cstdint>

namespace Automata
{

/**
 * @brief Replay payload structure.
 *
 * Binary layout (little-endian):
 *   [0..3]   magic "AWRP"
 *   [4..5]   ReplayVersion (uint16)
 *   [6..13]  RulesetHash (uint64)
 *   [14..21] seed (uint64)
 *   [22..23] sourceA length (uint16)
 *   [24..N]  sourceA bytes (UTF-8)
 *   [N..N+1] sourceB length (uint16)
 *   [N+2..M] sourceB bytes (UTF-8)
 *   [M..M+3] CRC-32 of all preceding bytes
 */
struct ReplayData
{
    uint16_t    version    = ReplayVersion;
    uint64_t    rulesetHash = RulesetHash;
    uint64_t    seed       = 0;
    std::string sourceA;
    std::string sourceB;
};

/** Error codes from replay decode. */
enum class ReplayError : uint8_t
{
    None,
    InvalidMagic,
    VersionMismatch,
    RulesetMismatch,
    Truncated,
    ChecksumFailed,
    Base64Invalid
};

/** Result of a replay decode operation. */
struct ReplayDecodeResult
{
    ReplayData  data;
    ReplayError error = ReplayError::None;

    /** True if decode succeeded. */
    bool Ok() const { return error == ReplayError::None; }
};

/**
 * @brief Encode a replay to compact binary.
 * @param data The replay payload.
 * @return Binary blob.
 */
std::vector<uint8_t> EncodeReplay(const ReplayData& data);

/**
 * @brief Decode a replay from binary.
 * @param bytes Raw binary data.
 * @return Decoded replay or error.
 */
ReplayDecodeResult DecodeReplay(const std::vector<uint8_t>& bytes);

/**
 * @brief Encode binary replay to base64 string.
 * @param bytes Raw binary.
 * @return Base64-encoded string.
 */
std::string ReplayToBase64(const std::vector<uint8_t>& bytes);

/**
 * @brief Decode base64 string to binary replay.
 * @param b64 Base64 string.
 * @param outBytes Output binary vector.
 * @return True on success.
 */
bool ReplayFromBase64(const std::string& b64, std::vector<uint8_t>& outBytes);

} // namespace Automata
