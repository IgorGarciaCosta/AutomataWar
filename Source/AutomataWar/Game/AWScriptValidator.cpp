#include "AWScriptValidator.h"

FAWValidationResult FAWScriptValidator::Validate(const FString& Source)
{
	FAWValidationResult Result;

	// UTF-8 byte length check
	FTCHARToUTF8 Utf8(*Source);
	if (Utf8.Length() > MaxSourceBytes)
	{
		Result.ErrorMessage = FString::Printf(TEXT("Source exceeds maximum %d UTF-8 bytes (got %d)."), MaxSourceBytes, Utf8.Length());
		return Result;
	}

	// Character count
	if (Source.Len() > MaxSourceChars)
	{
		Result.ErrorMessage = FString::Printf(TEXT("Source exceeds maximum %d characters (got %d)."), MaxSourceChars, Source.Len());
		return Result;
	}

	// Line count
	int32 LineCount = 1;
	int32 CurrentTokenLen = 0;
	for (int32 i = 0; i < Source.Len(); ++i)
	{
		TCHAR Ch = Source[i];
		if (!IsAllowedChar(Ch))
		{
			Result.ErrorMessage = FString::Printf(TEXT("Disallowed character 0x%04X at position %d."), static_cast<uint32>(Ch), i);
			return Result;
		}
		if (Ch == TEXT('\n'))
		{
			++LineCount;
			CurrentTokenLen = 0;
		}
		else if (Ch == TEXT(' ') || Ch == TEXT('\t') || Ch == TEXT(',') || Ch == TEXT(':') || Ch == TEXT('\r'))
		{
			CurrentTokenLen = 0;
		}
		else
		{
			++CurrentTokenLen;
			if (CurrentTokenLen > MaxIdentifierLength)
			{
				Result.ErrorMessage = FString::Printf(TEXT("Token exceeds maximum %d characters at line %d."), MaxIdentifierLength, LineCount);
				return Result;
			}
		}
	}

	if (LineCount > MaxLines)
	{
		Result.ErrorMessage = FString::Printf(TEXT("Source exceeds maximum %d lines (got %d)."), MaxLines, LineCount);
		return Result;
	}

	Result.bSuccess = true;
	return Result;
}

bool FAWScriptValidator::IsAllowedChar(TCHAR Ch)
{
	if (Ch >= 0x20 && Ch <= 0x7E) return true; // printable ASCII
	if (Ch == 0x09 || Ch == 0x0A || Ch == 0x0D) return true; // tab, LF, CR
	return false;
}
