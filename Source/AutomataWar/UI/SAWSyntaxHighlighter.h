#pragma once

/**
 * @file SAWSyntaxHighlighter.h
 * @brief Rich-text marshaller that applies syntax coloring to Automata assembly.
 *
 * Used as the text marshaller for the code editor's SMultiLineEditableText.
 * Tokenizes each line and assigns FRunInfo with style names that map to
 * registered text styles.
 */

#include "CoreMinimal.h"
#include "Framework/Text/ITextDecorator.h"
#include "Framework/Text/SyntaxHighlighterTextLayoutMarshaller.h"

/**
 * @brief Marshaller producing syntax-colored runs for Automata assembly source.
 *
 * Token categories:
 * - Instruction (MOVE, TURN, SCAN, FIRE, SHIELD, SET, IF, WAIT)
 * - Register (R0-R3, R_HP, R_ENEMY_DIST, R_ENEMY_DIR, R_ENERGY, R_TICK)
 * - Label (identifier followed by colon, or jump target in IF)
 * - Number (integer literals including negatives)
 * - Comment (semicolons to end of line)
 * - Comparison operators (==, !=, <, <=, >, >=)
 * - Default text
 */
class AUTOMATAWAR_API FAWSyntaxHighlighter : public FSyntaxHighlighterTextLayoutMarshaller
{
public:
    /** Build tokenizer rules and immutable text styles for Automata source. */
    FAWSyntaxHighlighter();
    virtual ~FAWSyntaxHighlighter() override = default;

    /** @return A shared marshaller instance ready for a multiline Slate editor. */
    static TSharedRef<FAWSyntaxHighlighter> Create();

protected:
    virtual void ParseTokens(const FString &SourceString, FTextLayout &TargetTextLayout,
                             TArray<FSyntaxTokenizer::FTokenizedLine> TokenizedLines) override;

private:
    /** Token types for internal classification. */
    enum class ETokenType : uint8
    {
        Instruction,
        Register,
        Label,
        Number,
        Comment,
        Comparison,
        Default
    };

    /** Classify a single token word. */
    static ETokenType ClassifyToken(const FString &Token, bool bIsFirstToken, bool bHasColon);

    /** Get the FTextBlockStyle for a token type. */
    const FTextBlockStyle &GetStyleForToken(ETokenType Type) const;

    /** Registered text block styles. */
    FTextBlockStyle InstructionStyle;
    FTextBlockStyle RegisterStyle;
    FTextBlockStyle LabelStyle;
    FTextBlockStyle NumberStyle;
    FTextBlockStyle CommentStyle;
    FTextBlockStyle ComparisonStyle;
    FTextBlockStyle DefaultStyle;
};
