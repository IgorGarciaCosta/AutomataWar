#pragma once

/**
 * @file AWScriptValidator.h
 * @brief Server-authoritative validation of submitted scripts.
 *
 * Enforces size limits, charset rules, line count, identifier length.
 * Used identically for local standalone and networked submission.
 */

#include "CoreMinimal.h"
#include "AWMatchTypes.h"

/** Static utility class for script source validation before compilation. */
struct FAWScriptValidator
{
    /** Maximum UTF-8 bytes per submitted script. */
    static constexpr int32 MaxSourceBytes = 8192;
    /** Maximum characters (codepoints) per script. */
    static constexpr int32 MaxSourceChars = 8192;
    /** Maximum lines (newline-separated). */
    static constexpr int32 MaxLines = 512;
    /** Maximum identifier/token length in characters. */
    static constexpr int32 MaxIdentifierLength = 32;

    /**
     * @brief Validate raw source text.
     * @param Source The source script text.
     * @return Validation result with error message on failure.
     */
    static FAWValidationResult Validate(const FString &Source);

    /**
     * @brief Check if a character is in the allowed set.
     * Allowed: printable ASCII 0x20-0x7E, tab (0x09), newline (0x0A), carriage return (0x0D).
     */
    static bool IsAllowedChar(TCHAR Ch);
};
