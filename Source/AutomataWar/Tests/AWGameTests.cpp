/**
 * @file AWGameTests.cpp
 * @brief Automation tests for game layer: validation, phase transitions, replay safety.
 */

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AutomataWar/Game/AWScriptValidator.h"
#include "AutomataWar/Game/AWReplayService.h"
#include "AutomataWar/Game/AWExampleScripts.h"
#include "AutomataWar/Core/Lang/AutomataCompiler.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"

// ═══════════════════════════════════════════════════════════════════════════════
// Script Validator Tests
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValidatorAcceptsValid, "AutomataWar.Game.Validator.AcceptsValid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorAcceptsValid::RunTest(const FString& Parameters)
{
	FAWValidationResult R = FAWScriptValidator::Validate(TEXT("MOVE FWD\nFIRE\nWAIT\n"));
	TestTrue(TEXT("Valid script accepted"), R.bSuccess);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValidatorRejectsOversized, "AutomataWar.Game.Validator.RejectsOversized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorRejectsOversized::RunTest(const FString& Parameters)
{
	// Generate a string exceeding MaxSourceBytes
	FString Big;
	Big.Reserve(FAWScriptValidator::MaxSourceBytes + 100);
	for (int32 i = 0; i < FAWScriptValidator::MaxSourceBytes + 100; ++i)
	{
		Big.AppendChar(TEXT('A'));
	}
	FAWValidationResult R = FAWScriptValidator::Validate(Big);
	TestFalse(TEXT("Oversized rejected"), R.bSuccess);
	TestTrue(TEXT("Error mentions bytes or characters"), R.ErrorMessage.Contains(TEXT("maximum")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValidatorRejectsTooManyLines, "AutomataWar.Game.Validator.RejectsTooManyLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorRejectsTooManyLines::RunTest(const FString& Parameters)
{
	FString ManyLines;
	for (int32 i = 0; i <= FAWScriptValidator::MaxLines; ++i)
	{
		ManyLines += TEXT("MOVE FWD\n");
	}
	FAWValidationResult R = FAWScriptValidator::Validate(ManyLines);
	TestFalse(TEXT("Too many lines rejected"), R.bSuccess);
	TestTrue(TEXT("Error mentions lines"), R.ErrorMessage.Contains(TEXT("lines")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValidatorRejectsLongIdentifier, "AutomataWar.Game.Validator.RejectsLongIdentifier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorRejectsLongIdentifier::RunTest(const FString& Parameters)
{
	FString LongToken;
	for (int32 i = 0; i < FAWScriptValidator::MaxIdentifierLength + 5; ++i)
	{
		LongToken.AppendChar(TEXT('X'));
	}
	LongToken += TEXT("\n");
	FAWValidationResult R = FAWScriptValidator::Validate(LongToken);
	TestFalse(TEXT("Long identifier rejected"), R.bSuccess);
	TestTrue(TEXT("Error mentions token"), R.ErrorMessage.Contains(TEXT("Token")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValidatorRejectsBadCharset, "AutomataWar.Game.Validator.RejectsBadCharset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorRejectsBadCharset::RunTest(const FString& Parameters)
{
	// Null byte embedded
	FString Bad = TEXT("MOVE");
	Bad.AppendChar(0x01);
	Bad += TEXT("\n");
	FAWValidationResult R = FAWScriptValidator::Validate(Bad);
	TestFalse(TEXT("Bad charset rejected"), R.bSuccess);
	TestTrue(TEXT("Error mentions character"), R.ErrorMessage.Contains(TEXT("character")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FValidatorAllowsTabsAndCR, "AutomataWar.Game.Validator.AllowsTabsAndCR",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorAllowsTabsAndCR::RunTest(const FString& Parameters)
{
	FAWValidationResult R = FAWScriptValidator::Validate(TEXT("\tMOVE FWD\r\nFIRE\n"));
	TestTrue(TEXT("Tabs and CR allowed"), R.bSuccess);
	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Replay Service Path Safety Tests
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReplaySanitizeTraversal, "AutomataWar.Game.Replay.SanitizeTraversal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReplaySanitizeTraversal::RunTest(const FString& Parameters)
{
	// Path traversal attempts
	FString Safe = FAWReplayService::SanitizeFilename(TEXT("../../etc/passwd"));
	TestFalse(TEXT("No dots"), Safe.Contains(TEXT(".")));
	TestFalse(TEXT("No slashes"), Safe.Contains(TEXT("/")));
	TestFalse(TEXT("No backslashes"), Safe.Contains(TEXT("\\")));
	TestTrue(TEXT("Not empty"), Safe.Len() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReplaySanitizeSpecialChars, "AutomataWar.Game.Replay.SanitizeSpecialChars",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReplaySanitizeSpecialChars::RunTest(const FString& Parameters)
{
	FString Safe = FAWReplayService::SanitizeFilename(TEXT("my<replay>:\"test|?*"));
	TestEqual(TEXT("Only safe chars"), Safe, FString(TEXT("myreplaytest")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReplaySanitizeEmpty, "AutomataWar.Game.Replay.SanitizeEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReplaySanitizeEmpty::RunTest(const FString& Parameters)
{
	FString Safe = FAWReplayService::SanitizeFilename(TEXT("///"));
	TestEqual(TEXT("Falls back to 'replay'"), Safe, FString(TEXT("replay")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReplaySanitizeLength, "AutomataWar.Game.Replay.SanitizeLength",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReplaySanitizeLength::RunTest(const FString& Parameters)
{
	FString Long;
	for (int32 i = 0; i < 200; ++i) Long.AppendChar(TEXT('a'));
	FString Safe = FAWReplayService::SanitizeFilename(Long);
	TestTrue(TEXT("Clamped to 64"), Safe.Len() <= 64);
	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Example Scripts Compile Tests
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExampleScriptsCompile, "AutomataWar.Game.Examples.AllCompile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FExampleScriptsCompile::RunTest(const FString& Parameters)
{
	auto TestScript = [this](const FString& Name, const FString& Source)
	{
		std::string Src = TCHAR_TO_UTF8(*Source);
		Automata::CompileResult R = Automata::Compile(Src);
		TestTrue(*FString::Printf(TEXT("%s compiles"), *Name), R.Ok());
	};

	TestScript(TEXT("Aggressor"), FAWExampleScripts::Aggressor());
	TestScript(TEXT("Camper"), FAWExampleScripts::Camper());
	TestScript(TEXT("Kiter"), FAWExampleScripts::Kiter());
	TestScript(TEXT("DefaultBot"), FAWExampleScripts::DefaultBot());
	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Desync Detector Test
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDesyncDetectorMatches, "AutomataWar.Net.DesyncDetector.MatchesWhenDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDesyncDetectorMatches::RunTest(const FString& Parameters)
{
	// Run simulation twice with same seed, verify same hash
	FString Src0 = FAWExampleScripts::Aggressor();
	FString Src1 = FAWExampleScripts::Camper();

	std::string S0 = TCHAR_TO_UTF8(*Src0);
	std::string S1 = TCHAR_TO_UTF8(*Src1);
	Automata::CompileResult C0 = Automata::Compile(S0);
	Automata::CompileResult C1 = Automata::Compile(S1);
	TestTrue(TEXT("Both compile"), C0.Ok() && C1.Ok());

	Automata::SimConfig Config;
	Config.seed = 99999;

	Automata::Simulation SimA;
	SimA.RunMatch(C0.program, C1.program, Config);
	uint64 HashA = SimA.GetFinalHash();

	Automata::Simulation SimB;
	SimB.RunMatch(C0.program, C1.program, Config);
	uint64 HashB = SimB.GetFinalHash();

	TestEqual(TEXT("Deterministic hash"), HashA, HashB);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
