#include "AWReplayService.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogAutomataGame);

FString FAWReplayService::GetReplayDir()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Replays"));
}

FString FAWReplayService::SanitizeFilename(const FString& Raw)
{
	FString Safe;
	Safe.Reserve(Raw.Len());
	for (TCHAR Ch : Raw)
	{
		if (FChar::IsAlnum(Ch) || Ch == TEXT('_') || Ch == TEXT('-'))
		{
			Safe.AppendChar(Ch);
		}
	}
	if (Safe.IsEmpty())
	{
		Safe = TEXT("replay");
	}
	// Clamp length
	if (Safe.Len() > 64)
	{
		Safe.LeftInline(64);
	}
	return Safe;
}

bool FAWReplayService::Save(const FString& Filename, const Automata::ReplayData& Data)
{
	FString Dir = GetReplayDir();
	IFileManager::Get().MakeDirectory(*Dir, true);

	FString SafeName = SanitizeFilename(Filename);
	FString FullPath = FPaths::Combine(Dir, SafeName + TEXT(".awrp"));

	std::vector<uint8_t> Bytes = Automata::EncodeReplay(Data);
	TArray<uint8> UEBytes;
	UEBytes.Append(Bytes.data(), static_cast<int32>(Bytes.size()));

	return FFileHelper::SaveArrayToFile(UEBytes, *FullPath);
}

void FAWReplayService::List(TArray<FAWReplayInfo>& OutInfos)
{
	OutInfos.Reset();
	FString Dir = GetReplayDir();

	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(Dir, TEXT("*.awrp")), true, false);

	for (const FString& File : Files)
	{
		FString FullPath = FPaths::Combine(Dir, File);
		FFileStatData Stat = IFileManager::Get().GetStatData(*FullPath);

		FAWReplayInfo Info;
		Info.Filename = FPaths::GetBaseFilename(File);
		Info.Timestamp = Stat.ModificationTime;
		Info.FileSizeBytes = static_cast<int32>(Stat.FileSize);
		OutInfos.Add(MoveTemp(Info));
	}
}

bool FAWReplayService::Load(const FString& Filename, Automata::ReplayData& OutData, FString& OutError)
{
	FString SafeName = SanitizeFilename(Filename);
	FString FullPath = FPaths::Combine(GetReplayDir(), SafeName + TEXT(".awrp"));

	TArray<uint8> UEBytes;
	if (!FFileHelper::LoadFileToArray(UEBytes, *FullPath))
	{
		OutError = FString::Printf(TEXT("File not found: %s"), *SafeName);
		return false;
	}

	std::vector<uint8_t> Bytes(UEBytes.GetData(), UEBytes.GetData() + UEBytes.Num());
	Automata::ReplayDecodeResult Res = Automata::DecodeReplay(Bytes);
	if (!Res.Ok())
	{
		OutError = FString::Printf(TEXT("Decode error %d for %s"), static_cast<int>(Res.error), *SafeName);
		return false;
	}

	OutData = MoveTemp(Res.data);
	return true;
}

bool FAWReplayService::Delete(const FString& Filename)
{
	FString SafeName = SanitizeFilename(Filename);
	FString FullPath = FPaths::Combine(GetReplayDir(), SafeName + TEXT(".awrp"));
	return IFileManager::Get().Delete(*FullPath);
}

bool FAWReplayService::ExportBase64(const FString& Filename, FString& OutBase64)
{
	FString SafeName = SanitizeFilename(Filename);
	FString FullPath = FPaths::Combine(GetReplayDir(), SafeName + TEXT(".awrp"));

	TArray<uint8> UEBytes;
	if (!FFileHelper::LoadFileToArray(UEBytes, *FullPath))
	{
		return false;
	}

	std::vector<uint8_t> Bytes(UEBytes.GetData(), UEBytes.GetData() + UEBytes.Num());
	OutBase64 = UTF8_TO_TCHAR(Automata::ReplayToBase64(Bytes).c_str());
	return true;
}

bool FAWReplayService::ImportBase64(const FString& Base64, const FString& Filename, FString& OutError)
{
	std::string B64Str = TCHAR_TO_UTF8(*Base64);
	std::vector<uint8_t> Bytes;
	if (!Automata::ReplayFromBase64(B64Str, Bytes))
	{
		OutError = TEXT("Invalid base64 encoding.");
		return false;
	}

	// Validate the decoded data
	Automata::ReplayDecodeResult Res = Automata::DecodeReplay(Bytes);
	if (!Res.Ok())
	{
		OutError = FString::Printf(TEXT("Replay data invalid (error %d)."), static_cast<int>(Res.error));
		return false;
	}

	// Save to disk
	FString Dir = GetReplayDir();
	IFileManager::Get().MakeDirectory(*Dir, true);

	FString SafeName = SanitizeFilename(Filename);
	FString FullPath = FPaths::Combine(Dir, SafeName + TEXT(".awrp"));

	TArray<uint8> UEBytes;
	UEBytes.Append(Bytes.data(), static_cast<int32>(Bytes.size()));
	if (!FFileHelper::SaveArrayToFile(UEBytes, *FullPath))
	{
		OutError = TEXT("Failed to write file.");
		return false;
	}

	return true;
}
