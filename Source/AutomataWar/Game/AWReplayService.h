#pragma once

/**
 * @file AWReplayService.h
 * @brief Disk-backed replay storage: save, list, load, delete, import/export base64.
 *
 * All replays stored under Saved/Replays/ as .awrp binary files.
 * Filenames are sanitized against path traversal. Typical replay < 4KB.
 */

#include "CoreMinimal.h"
#include "AWMatchTypes.h"
#include "AutomataWar/Core/Replay/AutomataReplay.h"

/**
 * @brief Service for persisting and retrieving replay files.
 */
class FAWReplayService
{
public:
	/** Get the replays directory (Saved/Replays). */
	static FString GetReplayDir();

	/**
	 * @brief Save replay data to a named file.
	 * @param Filename Base filename (no path, no extension). Sanitized.
	 * @param Data The replay payload.
	 * @return True on success.
	 */
	static bool Save(const FString& Filename, const Automata::ReplayData& Data);

	/**
	 * @brief List all replay files.
	 * @param OutInfos Populated with replay file info.
	 */
	static void List(TArray<FAWReplayInfo>& OutInfos);

	/**
	 * @brief Load a replay by filename.
	 * @param Filename Base filename (sanitized).
	 * @param OutData Populated on success.
	 * @param OutError Error message on failure.
	 * @return True on success.
	 */
	static bool Load(const FString& Filename, Automata::ReplayData& OutData, FString& OutError);

	/**
	 * @brief Delete a replay file.
	 * @param Filename Base filename (sanitized).
	 * @return True if file was deleted.
	 */
	static bool Delete(const FString& Filename);

	/**
	 * @brief Export a replay to base64 string.
	 * @param Filename Base filename.
	 * @param OutBase64 Populated on success.
	 * @return True on success.
	 */
	static bool ExportBase64(const FString& Filename, FString& OutBase64);

	/**
	 * @brief Import a replay from base64 string and save to disk.
	 * @param Base64 The base64-encoded replay.
	 * @param Filename Destination filename (sanitized).
	 * @param OutError Error message on failure.
	 * @return True on success.
	 */
	static bool ImportBase64(const FString& Base64, const FString& Filename, FString& OutError);

	/**
	 * @brief Sanitize a filename removing path separators and traversal attempts.
	 * @param Raw Input name.
	 * @return Safe filename (alphanumeric, underscore, hyphen only).
	 */
	static FString SanitizeFilename(const FString& Raw);
};
