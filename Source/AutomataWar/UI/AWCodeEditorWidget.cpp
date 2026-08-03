#include "AWCodeEditorWidget.h"
#include "SAWCodeEditor.h"

FString UAWCodeEditorWidget::GetSourceText() const
{
    return CodeEditor.IsValid() ? CodeEditor->GetSourceText() : InitialText.ToString();
}

void UAWCodeEditorWidget::SetSourceText(const FString &Source)
{
    InitialText = FText::FromString(Source);
    if (CodeEditor.IsValid())
    {
        CodeEditor->SetSourceText(Source);
    }
}

bool UAWCodeEditorWidget::IsCompileOk() const
{
    return CodeEditor.IsValid() && CodeEditor->IsCompileOk();
}

TSharedRef<SWidget> UAWCodeEditorWidget::RebuildWidget()
{
    return SAssignNew(CodeEditor, SAWCodeEditor)
        .InitialText(InitialText)
        .IsReadOnly(bReadOnly);
}

void UAWCodeEditorWidget::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    CodeEditor.Reset();
}