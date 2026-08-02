/**
 * @file SAWSyntaxHighlighter.cpp
 * @brief Implementation of the Automata assembly syntax highlighter marshaller.
 */

#include "SAWSyntaxHighlighter.h"
#include "AWUITypes.h"
#include "Framework/Text/SlateTextRun.h"
#include "Framework/Text/TextLayout.h"
#include "Misc/DefaultValueHelper.h"

namespace
{
	/** Set of known instruction mnemonics (uppercased). */
	static const TSet<FString> InstructionSet = {
		TEXT("MOVE"), TEXT("TURN"), TEXT("SCAN"), TEXT("FIRE"),
		TEXT("SHIELD"), TEXT("SET"), TEXT("IF"), TEXT("WAIT"),
		TEXT("FWD"), TEXT("BACK"), TEXT("LEFT"), TEXT("RIGHT"), TEXT("JUMP")
	};

	/** Set of known register names (uppercased). */
	static const TSet<FString> RegisterSet = {
		TEXT("R0"), TEXT("R1"), TEXT("R2"), TEXT("R3"),
		TEXT("R_HP"), TEXT("R_ENEMY_DIST"), TEXT("R_ENEMY_DIR"),
		TEXT("R_ENERGY"), TEXT("R_TICK")
	};

	/** Set of comparison operators. */
	static const TSet<FString> ComparisonSet = {
		TEXT("=="), TEXT("!="), TEXT("<"), TEXT("<="), TEXT(">"), TEXT(">=")
	};

	FTextBlockStyle MakeStyle(const FLinearColor& Color)
	{
		FTextBlockStyle Style = FTextBlockStyle::GetDefault();
		Style.SetColorAndOpacity(FSlateColor(Color));
		return Style;
	}
}

FAWSyntaxHighlighter::FAWSyntaxHighlighter()
	: FSyntaxHighlighterTextLayoutMarshaller(FSyntaxTokenizer::Create(TArray<FSyntaxTokenizer::FRule>()))
{
	InstructionStyle = MakeStyle(AWUIColors::SyntaxInstruction);
	RegisterStyle = MakeStyle(AWUIColors::SyntaxRegister);
	LabelStyle = MakeStyle(AWUIColors::SyntaxLabel);
	NumberStyle = MakeStyle(AWUIColors::SyntaxNumber);
	CommentStyle = MakeStyle(AWUIColors::SyntaxComment);
	ComparisonStyle = MakeStyle(AWUIColors::AccentCyan);
	DefaultStyle = MakeStyle(AWUIColors::TextPrimary);
}

TSharedRef<FAWSyntaxHighlighter> FAWSyntaxHighlighter::Create()
{
	return MakeShareable(new FAWSyntaxHighlighter());
}

FAWSyntaxHighlighter::ETokenType FAWSyntaxHighlighter::ClassifyToken(const FString& Token, bool bIsFirstToken, bool bHasColon)
{
	if (Token.IsEmpty()) return ETokenType::Default;

	// Check if it's a label definition (has trailing colon removed by caller already)
	if (bHasColon && bIsFirstToken)
	{
		return ETokenType::Label;
	}

	FString Upper = Token.ToUpper();
	Upper.RemoveFromEnd(TEXT(","));
	Upper.RemoveFromEnd(TEXT(":"));

	if (InstructionSet.Contains(Upper))
	{
		return ETokenType::Instruction;
	}

	if (RegisterSet.Contains(Upper))
	{
		return ETokenType::Register;
	}

	if (ComparisonSet.Contains(Token))
	{
		return ETokenType::Comparison;
	}

	// Number check: optional minus followed by digits
	FString NumCheck = Token;
	NumCheck.RemoveFromEnd(TEXT(","));
	if (NumCheck.Len() > 0)
	{
		bool bIsNumber = true;
		int32 Start = 0;
		if (NumCheck[0] == TEXT('-')) Start = 1;
		if (Start >= NumCheck.Len()) bIsNumber = false;
		for (int32 i = Start; i < NumCheck.Len() && bIsNumber; ++i)
		{
			if (!FChar::IsDigit(NumCheck[i])) bIsNumber = false;
		}
		if (bIsNumber) return ETokenType::Number;
	}

	// If it looks like a label reference (used in IF as jump target, or instruction labels)
	// Non-first tokens that are plain identifiers ending without comma may be labels
	if (!bIsFirstToken)
	{
		bool bAllAlphaUnder = true;
		for (TCHAR Ch : Token)
		{
			if (!FChar::IsAlpha(Ch) && Ch != TEXT('_') && !FChar::IsDigit(Ch))
			{
				bAllAlphaUnder = false;
				break;
			}
		}
		if (bAllAlphaUnder && !Upper.IsEmpty() && FChar::IsAlpha(Upper[0])
			&& !InstructionSet.Contains(Upper) && !RegisterSet.Contains(Upper))
		{
			return ETokenType::Label;
		}
	}

	return ETokenType::Default;
}

const FTextBlockStyle& FAWSyntaxHighlighter::GetStyleForToken(ETokenType Type) const
{
	switch (Type)
	{
	case ETokenType::Instruction: return InstructionStyle;
	case ETokenType::Register:    return RegisterStyle;
	case ETokenType::Label:       return LabelStyle;
	case ETokenType::Number:      return NumberStyle;
	case ETokenType::Comment:     return CommentStyle;
	case ETokenType::Comparison:  return ComparisonStyle;
	default:                      return DefaultStyle;
	}
}

void FAWSyntaxHighlighter::ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout,
	TArray<FSyntaxTokenizer::FTokenizedLine> TokenizedLines)
{
	// Override the token system: parse source line-by-line ourselves.
	TArray<FString> Lines;
	SourceString.ParseIntoArray(Lines, TEXT("\n"), false);

	TArray<FTextLayout::FNewLineData> LinesToAdd;
	LinesToAdd.Reserve(Lines.Num());

	for (const FString& Line : Lines)
	{
		TSharedRef<FString> ModelString = MakeShareable(new FString());
		TArray<TSharedRef<IRun>> Runs;

		// Check for comment: everything from ';' onward
		int32 CommentIdx = INDEX_NONE;
		Line.FindChar(TEXT(';'), CommentIdx);

		FString CodePart = (CommentIdx != INDEX_NONE) ? Line.Left(CommentIdx) : Line;
		FString CommentPart = (CommentIdx != INDEX_NONE) ? Line.Mid(CommentIdx) : FString();

		// Tokenize code part by whitespace
		TArray<FString> Tokens;
		CodePart.ParseIntoArrayWS(Tokens);

		int32 CurrentPos = 0;
		bool bFirst = true;

		for (const FString& RawToken : Tokens)
		{
			// Find token position in original Line
			int32 TokenStart = Line.Find(RawToken, ESearchCase::CaseSensitive, ESearchDir::FromStart, CurrentPos);
			if (TokenStart == INDEX_NONE) TokenStart = CurrentPos;

			// Leading whitespace
			if (TokenStart > CurrentPos)
			{
				FString Space = Line.Mid(CurrentPos, TokenStart - CurrentPos);
				FTextRange Range(ModelString->Len(), ModelString->Len() + Space.Len());
				ModelString->Append(Space);
				Runs.Add(FSlateTextRun::Create(FRunInfo(), ModelString, DefaultStyle, Range));
			}

			bool bHasColon = RawToken.Contains(TEXT(":"));
			ETokenType Type = ClassifyToken(RawToken, bFirst, bHasColon);
			const FTextBlockStyle& Style = GetStyleForToken(Type);

			FTextRange Range(ModelString->Len(), ModelString->Len() + RawToken.Len());
			ModelString->Append(RawToken);
			Runs.Add(FSlateTextRun::Create(FRunInfo(), ModelString, Style, Range));

			CurrentPos = TokenStart + RawToken.Len();
			bFirst = false;
		}

		// Gap between code and comment
		if (CommentIdx != INDEX_NONE && CurrentPos < CommentIdx)
		{
			FString Gap = Line.Mid(CurrentPos, CommentIdx - CurrentPos);
			FTextRange Range(ModelString->Len(), ModelString->Len() + Gap.Len());
			ModelString->Append(Gap);
			Runs.Add(FSlateTextRun::Create(FRunInfo(), ModelString, DefaultStyle, Range));
			CurrentPos = CommentIdx;
		}

		// Comment part
		if (!CommentPart.IsEmpty())
		{
			FTextRange Range(ModelString->Len(), ModelString->Len() + CommentPart.Len());
			ModelString->Append(CommentPart);
			Runs.Add(FSlateTextRun::Create(FRunInfo(), ModelString, CommentStyle, Range));
			CurrentPos += CommentPart.Len();
		}

		// Trailing whitespace
		if (CurrentPos < Line.Len())
		{
			FString Trail = Line.Mid(CurrentPos);
			FTextRange Range(ModelString->Len(), ModelString->Len() + Trail.Len());
			ModelString->Append(Trail);
			Runs.Add(FSlateTextRun::Create(FRunInfo(), ModelString, DefaultStyle, Range));
		}

		// Empty line needs at least one run
		if (Runs.Num() == 0)
		{
			Runs.Add(FSlateTextRun::Create(FRunInfo(), ModelString, DefaultStyle, FTextRange(0, 0)));
		}

		LinesToAdd.Add(FTextLayout::FNewLineData(MoveTemp(ModelString), MoveTemp(Runs)));
	}

	TargetTextLayout.AddLines(LinesToAdd);
}
