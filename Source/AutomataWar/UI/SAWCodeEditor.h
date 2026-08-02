#pragma once

/**
 * @file SAWCodeEditor.h
 * @brief Custom Slate multiline code editor with line numbers, syntax highlighting,
 *        debounced compilation, inline error indication, and diagnostic panel.
 *
 * Implements a complete editing surface for the Automata assembly language.
 * Syntax coloring covers instructions, registers, labels, numbers, and comments.
 */

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Framework/Text/IRun.h"
#include "AutomataWar/Core/Lang/AutomataCompiler.h"

class SScrollBar;
class SVerticalBox;

/**
 * @brief Slate widget providing a syntax-highlighted code editor with diagnostics.
 *
 * Features:
 * - Visible line numbers in left gutter
 * - Multiline editable text with monospace font
 * - Syntax colors for opcodes, registers, labels, numbers, comments
 * - Debounced compile-on-edit (300ms after last keystroke)
 * - Status indicator (OK / Error count)
 * - Diagnostic list with line/column/message
 * - Error lines highlighted with red tint in gutter
 */
class AUTOMATAWAR_API SAWCodeEditor : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAWCodeEditor)
        : _InitialText(FText::GetEmpty()), _IsReadOnly(false)
    {
    }
    SLATE_ARGUMENT(FText, InitialText)
    SLATE_ARGUMENT(bool, IsReadOnly)
    SLATE_EVENT(FSimpleDelegate, OnCompileSuccess)
    SLATE_END_ARGS()

    void Construct(const FArguments &InArgs);

    /** Get the current source text. */
    FString GetSourceText() const;

    /** Set source text programmatically. */
    void SetSourceText(const FString &InText);

    /** Get the latest compile result (diagnostics). */
    const Automata::CompileResult &GetCompileResult() const { return LastCompileResult; }

    /** Returns true if last compilation had zero errors. */
    bool IsCompileOk() const { return LastCompileResult.Ok(); }

private:
    /** Called when text changes. Starts debounce timer. */
    void OnTextChanged(const FText &NewText);

    /** Actually compile the current text. */
    void DoCompile();

    /** Rebuild the gutter line-number display. */
    void RebuildGutter();

    /** Rebuild the diagnostics panel. */
    void RebuildDiagnostics();

    TSharedPtr<SMultiLineEditableTextBox> TextEditor;
    TSharedPtr<SVerticalBox> GutterBox;
    TSharedPtr<SVerticalBox> DiagnosticsBox;
    TSharedPtr<SScrollBar> ScrollBar;

    FText CurrentText;
    Automata::CompileResult LastCompileResult;
    FSimpleDelegate OnCompileSuccess;
    bool bReadOnly = false;

    /** Debounce timer handle. */
    FTimerHandle DebounceHandle;
    /** Time of last text change for debounce. */
    double LastEditTime = 0.0;
};
