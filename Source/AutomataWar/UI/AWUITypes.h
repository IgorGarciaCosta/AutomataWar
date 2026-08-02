#pragma once

/**
 * @file AWUITypes.h
 * @brief Shared types, log category, and asset path constants for Automata War UI.
 */

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAutomataUI, Log, All);

/** Centralized soft-path references for optional UI assets. */
namespace AWUIAssets
{
    /** Project monospace font file, staged as loose content for Slate. */
    inline const TCHAR *MonoFontFile = TEXT("UI/Fonts/F_AWMono.ttf");
    /** Project display font file, staged as loose content for Slate. */
    inline const TCHAR *DisplayFontFile = TEXT("UI/Fonts/F_AWDisplay.ttf");
    /** Optional imported project font asset path. */
    inline const TCHAR *MonoFontPath = TEXT("/Game/UI/Fonts/F_AWMono.F_AWMono");
    /** Engine fallback monospace font. */
    inline const TCHAR *FallbackMonoFontPath = TEXT("/Engine/EngineFonts/RobotoMono.RobotoMono");
    /** Optional click/confirm SFX. */
    inline const TCHAR *SFX_UIConfirm = TEXT("/Game/Audio/SFX/S_UIConfirm.S_UIConfirm");
    /** Optional navigate SFX. */
    inline const TCHAR *SFX_UINavigate = TEXT("/Game/Audio/SFX/S_UINavigate.S_UINavigate");
    /** Error/rejection SFX. */
    inline const TCHAR *SFX_UIError = TEXT("/Game/Audio/SFX/S_UIError.S_UIError");
}

/** Font helpers backed by the project's licensed font files. */
namespace AWUIFonts
{
    /** Return Roboto Mono at the requested point size. */
    AUTOMATAWAR_API FSlateFontInfo Mono(int32 Size);
    /** Return Rajdhani SemiBold at the requested point size. */
    AUTOMATAWAR_API FSlateFontInfo Display(int32 Size);
}

/** Color palette for the sci-fi tool aesthetic. */
namespace AWUIColors
{
    inline const FLinearColor Background{0.02f, 0.025f, 0.035f, 1.f};
    inline const FLinearColor Panel{0.04f, 0.05f, 0.065f, 1.f};
    inline const FLinearColor PanelBorder{0.12f, 0.15f, 0.2f, 1.f};
    inline const FLinearColor TextPrimary{0.9f, 0.92f, 0.95f, 1.f};
    inline const FLinearColor TextSecondary{0.55f, 0.58f, 0.62f, 1.f};
    inline const FLinearColor AccentCyan{0.0f, 0.85f, 0.95f, 1.f};
    inline const FLinearColor AccentCoral{1.0f, 0.35f, 0.3f, 1.f};
    inline const FLinearColor Separator{0.1f, 0.12f, 0.16f, 1.f};
    inline const FLinearColor ErrorRed{1.f, 0.2f, 0.2f, 1.f};
    inline const FLinearColor SuccessGreen{0.2f, 0.9f, 0.4f, 1.f};
    inline const FLinearColor WarningYellow{1.f, 0.8f, 0.2f, 1.f};
    /** Syntax color: instructions/opcodes. */
    inline const FLinearColor SyntaxInstruction{0.4f, 0.7f, 1.f, 1.f};
    /** Syntax color: registers. */
    inline const FLinearColor SyntaxRegister{0.9f, 0.6f, 0.2f, 1.f};
    /** Syntax color: labels. */
    inline const FLinearColor SyntaxLabel{0.5f, 0.9f, 0.5f, 1.f};
    /** Syntax color: numeric literals. */
    inline const FLinearColor SyntaxNumber{0.85f, 0.4f, 0.85f, 1.f};
    /** Syntax color: comments. */
    inline const FLinearColor SyntaxComment{0.45f, 0.5f, 0.55f, 1.f};
}
