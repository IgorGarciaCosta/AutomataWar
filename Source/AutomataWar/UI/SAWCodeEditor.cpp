/**
 * @file SAWCodeEditor.cpp
 * @brief Implementation of the Slate code editor widget with syntax highlighting and diagnostics.
 */

#include "SAWCodeEditor.h"
#include "SAWSyntaxHighlighter.h"
#include "AWUITypes.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/SBoxPanel.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void SAWCodeEditor::Construct(const FArguments &InArgs)
{
    CurrentText = InArgs._InitialText;
    bReadOnly = InArgs._IsReadOnly;
    OnCompileSuccess = InArgs._OnCompileSuccess;

    FSlateFontInfo MonoFont = AWUIFonts::Mono(12);

    ChildSlot
        [SNew(SVerticalBox)
         // Editor area
         + SVerticalBox::Slot()
               .FillHeight(1.f)
                   [SNew(SHorizontalBox)
                    // Gutter (line numbers)
                    + SHorizontalBox::Slot()
                          .AutoWidth()
                          .Padding(0, 0, 4, 0)
                              [SNew(SBorder)
                                   .BorderBackgroundColor(AWUIColors::Panel)
                                   .Padding(FMargin(4, 2))
                                       [SAssignNew(GutterBox, SVerticalBox)]]
                    // Code editor
                    + SHorizontalBox::Slot()
                          .FillWidth(1.f)
                              [SNew(SBorder)
                                   .BorderBackgroundColor(AWUIColors::Background)
                                   .Padding(FMargin(4, 2))
                                       [SAssignNew(TextEditor, SMultiLineEditableTextBox)
                                            .Font(MonoFont)
                                            .Text(CurrentText)
                                            .IsReadOnly(bReadOnly)
                                            .OnTextChanged(this, &SAWCodeEditor::OnTextChanged)
                                            .Marshaller(FAWSyntaxHighlighter::Create())]]]
         // Separator
         + SVerticalBox::Slot()
               .AutoHeight()
               .Padding(0, 2)
                   [SNew(SSeparator)
                        .SeparatorImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                        .ColorAndOpacity(AWUIColors::Separator)]
         // Diagnostics panel
         + SVerticalBox::Slot()
               .AutoHeight()
               .MaxHeight(120.f)
                   [SNew(SScrollBox) + SScrollBox::Slot()
                                           [SAssignNew(DiagnosticsBox, SVerticalBox)]]];

    RebuildGutter();
    // Initial compile
    DoCompile();
}

FString SAWCodeEditor::GetSourceText() const
{
    if (TextEditor.IsValid())
    {
        return TextEditor->GetText().ToString();
    }
    return CurrentText.ToString();
}

void SAWCodeEditor::SetSourceText(const FString &InText)
{
    CurrentText = FText::FromString(InText);
    if (TextEditor.IsValid())
    {
        TextEditor->SetText(CurrentText);
    }
    DoCompile();
}

void SAWCodeEditor::OnTextChanged(const FText &NewText)
{
    CurrentText = NewText;
    LastEditTime = FPlatformTime::Seconds();
    RebuildGutter();

    // Debounce: compile 300ms after last keystroke
    // Use a simple tick-based approach since we don't have world timer in Slate
    // We'll compile on every change for correctness (debounce is nice-to-have in Slate)
    DoCompile();
}

void SAWCodeEditor::DoCompile()
{
    FString Source = CurrentText.ToString();
    std::string StdSrc = TCHAR_TO_UTF8(*Source);
    LastCompileResult = Automata::Compile(StdSrc);
    RebuildDiagnostics();
}

void SAWCodeEditor::RebuildGutter()
{
    if (!GutterBox.IsValid())
        return;
    GutterBox->ClearChildren();

    FString Source = CurrentText.ToString();
    int32 LineCount = 1;
    for (TCHAR Ch : Source)
    {
        if (Ch == TEXT('\n'))
            ++LineCount;
    }

    // Collect error lines
    TSet<int32> ErrorLines;
    for (const auto &D : LastCompileResult.diagnostics)
    {
        if (D.severity == Automata::DiagSeverity::Error)
        {
            ErrorLines.Add(D.line);
        }
    }

    FSlateFontInfo GutterFont = AWUIFonts::Mono(11);

    for (int32 i = 1; i <= LineCount; ++i)
    {
        FLinearColor LineColor = ErrorLines.Contains(i) ? AWUIColors::ErrorRed : AWUIColors::TextSecondary;

        GutterBox->AddSlot()
            .AutoHeight()
                [SNew(STextBlock)
                     .Font(GutterFont)
                     .Text(FText::AsNumber(i))
                     .ColorAndOpacity(FSlateColor(LineColor))
                     .Justification(ETextJustify::Right)
                     .MinDesiredWidth(30.f)];
    }
}

void SAWCodeEditor::RebuildDiagnostics()
{
    if (!DiagnosticsBox.IsValid())
        return;
    DiagnosticsBox->ClearChildren();

    if (LastCompileResult.diagnostics.empty())
    {
        DiagnosticsBox->AddSlot()
            .AutoHeight()
            .Padding(4, 2)
                [SNew(STextBlock)
                     .Text(FText::FromString(TEXT("OK")))
                     .ColorAndOpacity(FSlateColor(AWUIColors::SuccessGreen))];
        if (OnCompileSuccess.IsBound())
            OnCompileSuccess.Execute();
        return;
    }

    for (const auto &D : LastCompileResult.diagnostics)
    {
        FLinearColor Color = (D.severity == Automata::DiagSeverity::Error) ? AWUIColors::ErrorRed : AWUIColors::WarningYellow;
        FString Msg = FString::Printf(TEXT("L%d:%d  %s"), D.line, D.column,
                                      UTF8_TO_TCHAR(D.message.c_str()));

        DiagnosticsBox->AddSlot()
            .AutoHeight()
            .Padding(4, 1)
                [SNew(STextBlock)
                     .Text(FText::FromString(Msg))
                     .ColorAndOpacity(FSlateColor(Color))
                     .Font(AWUIFonts::Mono(10))];
    }

    RebuildGutter();
}
