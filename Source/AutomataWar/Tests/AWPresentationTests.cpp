/**
 * @file AWPresentationTests.cpp
 * @brief Automation tests for UI and Visual presentation layers.
 *
 * Tests: language reference count, replay debugger determinism,
 * asset soft-path centralization, widget construction sanity.
 */

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AutomataWar/Core/AutomataRules.h"
#include "AutomataWar/Core/Lang/AutomataCompiler.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"
#include "AutomataWar/Core/Replay/AutomataReplay.h"
#include "AutomataWar/Game/AWExampleScripts.h"
#include "AutomataWar/UI/AWUITypes.h"
#include "AutomataWar/UI/SAWCodeEditor.h"
#include "AutomataWar/Visual/AWVisualTypes.h"

// ═══════════════════════════════════════════════════════════════════════════════
// Language Reference Definitions Count
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLangRefInstructionCount, "AutomataWar.UI.LangRef.InstructionCountIs8",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLangRefInstructionCount::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("OpcodeCount == 8"), Automata::OpcodeCount, 8);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLangRefRegisterCount, "AutomataWar.UI.LangRef.RegisterCountIs9",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLangRefRegisterCount::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("TotalRegisterCount == 9"), Automata::TotalRegisterCount, 9);
	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Replay Debugger: Stepping backward/scrubbing re-sim yields same hashes
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReplayResimDeterminism, "AutomataWar.UI.Replay.ResimDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReplayResimDeterminism::RunTest(const FString& Parameters)
{
	FString Src0 = FAWExampleScripts::Aggressor();
	FString Src1 = FAWExampleScripts::Kiter();
	std::string S0 = TCHAR_TO_UTF8(*Src0);
	std::string S1 = TCHAR_TO_UTF8(*Src1);
	Automata::CompileResult C0 = Automata::Compile(S0);
	Automata::CompileResult C1 = Automata::Compile(S1);
	TestTrue(TEXT("Both compile"), C0.Ok() && C1.Ok());

	Automata::SimConfig Config;
	Config.seed = 42;

	// Full sim
	Automata::Simulation SimFull;
	SimFull.RunMatch(C0.program, C1.program, Config);
	const auto& FullSnaps = SimFull.GetSnapshots();
	TestTrue(TEXT("Has snapshots"), FullSnaps.size() > 10);

	// Re-sim (simulating "scrub to tick 0 and replay forward")
	Automata::Simulation SimResim;
	SimResim.RunMatch(C0.program, C1.program, Config);
	const auto& ResimSnaps = SimResim.GetSnapshots();

	// Compare every snapshot hash
	TestEqual(TEXT("Same snapshot count"), (int32)FullSnaps.size(), (int32)ResimSnaps.size());
	bool bAllMatch = true;
	for (size_t i = 0; i < FullSnaps.size() && i < ResimSnaps.size(); ++i)
	{
		if (FullSnaps[i].stateHash != ResimSnaps[i].stateHash)
		{
			bAllMatch = false;
			AddError(FString::Printf(TEXT("Hash mismatch at tick %d: %llu vs %llu"),
				(int32)i, FullSnaps[i].stateHash, ResimSnaps[i].stateHash));
			break;
		}
	}
	TestTrue(TEXT("All snapshot hashes match"), bAllMatch);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReplayStepBackResim, "AutomataWar.UI.Replay.StepBackResim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReplayStepBackResim::RunTest(const FString& Parameters)
{
	FString Src0 = FAWExampleScripts::Camper();
	FString Src1 = FAWExampleScripts::DefaultBot();
	std::string S0 = TCHAR_TO_UTF8(*Src0);
	std::string S1 = TCHAR_TO_UTF8(*Src1);
	Automata::CompileResult C0 = Automata::Compile(S0);
	Automata::CompileResult C1 = Automata::Compile(S1);
	TestTrue(TEXT("Both compile"), C0.Ok() && C1.Ok());

	Automata::SimConfig Config;
	Config.seed = 777;

	Automata::Simulation Sim;
	Sim.RunMatch(C0.program, C1.program, Config);
	const auto& Snaps = Sim.GetSnapshots();

	// Pick a mid-point tick and verify re-sim to that point gives same state
	if (Snaps.size() > 20)
	{
		int32 TargetTick = 20;
		uint64 ExpectedHash = Snaps[TargetTick].stateHash;

		// Re-sim
		Automata::Simulation Sim2;
		Sim2.RunMatch(C0.program, C1.program, Config);
		const auto& Snaps2 = Sim2.GetSnapshots();

		TestTrue(TEXT("Re-sim has enough ticks"), (int32)Snaps2.size() > TargetTick);
		if ((int32)Snaps2.size() > TargetTick)
		{
			TestEqual(TEXT("Step-back hash matches"), Snaps2[TargetTick].stateHash, ExpectedHash);
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Asset Soft Paths Centralized
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAssetPathsCentralized, "AutomataWar.Visual.AssetPaths.AreCentralized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAssetPathsCentralized::RunTest(const FString& Parameters)
{
	// Verify all asset paths are non-empty and follow UE path format
	auto CheckPath = [this](const TCHAR* Path, const TCHAR* Name)
	{
		FString P(Path);
		TestTrue(FString::Printf(TEXT("%s not empty"), Name), P.Len() > 0);
		TestTrue(FString::Printf(TEXT("%s starts with /"), Name), P.StartsWith(TEXT("/")));
		TestTrue(FString::Printf(TEXT("%s contains dot"), Name), P.Contains(TEXT(".")));
	};

	CheckPath(AWVisualAssets::NS_MuzzleFlash, TEXT("NS_MuzzleFlash"));
	CheckPath(AWVisualAssets::NS_ProjectileTrail, TEXT("NS_ProjectileTrail"));
	CheckPath(AWVisualAssets::NS_Impact, TEXT("NS_Impact"));
	CheckPath(AWVisualAssets::NS_Shield, TEXT("NS_Shield"));
	CheckPath(AWVisualAssets::NS_Destruction, TEXT("NS_Destruction"));
	CheckPath(AWVisualAssets::SFX_Fire, TEXT("SFX_Fire"));
	CheckPath(AWVisualAssets::SFX_Impact, TEXT("SFX_Impact"));
	CheckPath(AWVisualAssets::SFX_Shield, TEXT("SFX_Shield"));
	CheckPath(AWVisualAssets::SFX_Move, TEXT("SFX_Move"));
	CheckPath(AWVisualAssets::SFX_Destroy, TEXT("SFX_Destroy"));
	CheckPath(AWUIAssets::MonoFontPath, TEXT("MonoFontPath"));
	CheckPath(AWUIAssets::FallbackMonoFontPath, TEXT("FallbackMonoFontPath"));
	CheckPath(AWUIAssets::SFX_UIConfirm, TEXT("SFX_UIConfirm"));
	CheckPath(AWUIAssets::SFX_UINavigate, TEXT("SFX_UINavigate"));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Widget Construction Sanity (no crash on construct)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeEditorConstruct, "AutomataWar.UI.CodeEditor.Constructs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCodeEditorConstruct::RunTest(const FString& Parameters)
{
	// Verify SAWCodeEditor can be constructed without crash
	TSharedPtr<SAWCodeEditor> Editor;
	SAssignNew(Editor, SAWCodeEditor)
		.InitialText(FText::FromString(TEXT("MOVE\nFIRE\n")));

	TestTrue(TEXT("Editor constructed"), Editor.IsValid());
	TestEqual(TEXT("Source matches"), Editor->GetSourceText(), FString(TEXT("MOVE\nFIRE\n")));
	TestTrue(TEXT("Compiles OK"), Editor->IsCompileOk());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCodeEditorDiagnostics, "AutomataWar.UI.CodeEditor.ShowsDiagnostics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCodeEditorDiagnostics::RunTest(const FString& Parameters)
{
	TSharedPtr<SAWCodeEditor> Editor;
	SAssignNew(Editor, SAWCodeEditor)
		.InitialText(FText::FromString(TEXT("MAVE\n")));

	TestTrue(TEXT("Editor constructed"), Editor.IsValid());
	TestFalse(TEXT("Has compile errors"), Editor->IsCompileOk());
	TestTrue(TEXT("Has diagnostics"), Editor->GetCompileResult().diagnostics.size() > 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
