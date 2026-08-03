#pragma once

/**
 * @file AWCodeEditorWidget.h
 * @brief UMG-facing wrapper for the native Automata code editor.
 */

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AWCodeEditorWidget.generated.h"

class SAWCodeEditor;

/**
 * Blueprintable host for the syntax-highlighted Slate editor.
 *
 * Use this class as a Widget Blueprint parent when a designer-owned UMG asset
 * needs the native editor without duplicating compiler or diagnostic logic.
 */
UCLASS(Blueprintable)
class AUTOMATAWAR_API UAWCodeEditorWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Return the source currently displayed by the editor. */
    UFUNCTION(BlueprintPure, Category = "AutomataWar|Editor")
    FString GetSourceText() const;

    /** Replace the editor source text. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Editor")
    void SetSourceText(const FString &Source);

    /** Return whether the latest native compile completed without errors. */
    UFUNCTION(BlueprintPure, Category = "AutomataWar|Editor")
    bool IsCompileOk() const;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

    /** Source shown when the widget is first constructed. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutomataWar|Editor", meta = (MultiLine = "true"))
    FText InitialText;

    /** Prevent editing while retaining syntax highlighting and diagnostics. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AutomataWar|Editor")
    bool bReadOnly = false;

private:
    TSharedPtr<SAWCodeEditor> CodeEditor;
};