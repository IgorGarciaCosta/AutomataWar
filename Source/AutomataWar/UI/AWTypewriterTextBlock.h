#pragma once

/**
 * @file AWTypewriterTextBlock.h
 * @brief Reusable UMG text block with typewriter and caret animation.
 */

#include "CoreMinimal.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"
#include "AWTypewriterTextBlock.generated.h"

/**
 * Reveals its configured text one character at a time, then keeps a caret
 * blinking until the animation is replayed or the widget is released.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Typewriter Text"))
class AUTOMATAWAR_API UAWTypewriterTextBlock : public UTextBlock
{
    GENERATED_BODY()

public:
    /** Delay between PlayTypewriter and the first revealed character. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutomataWar|Typewriter", meta = (ClampMin = "0.0"))
    float InitialDelay = 0.5f;

    /** Delay between consecutive character reveals. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutomataWar|Typewriter", meta = (ClampMin = "0.001"))
    float SecondsPerCharacter = 0.075f;

    /** Delay between caret visibility changes after typing completes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutomataWar|Typewriter", meta = (ClampMin = "0.01"))
    float CaretBlinkInterval = 0.5f;

    /** Text rendered as the trailing caret. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutomataWar|Typewriter")
    FText CaretText = INVTEXT("|");

    /** Restarts the animation from an empty frame using the current target text. */
    UFUNCTION(BlueprintCallable, Category = "AutomataWar|Typewriter")
    void PlayTypewriter();

    /** Replaces the target text and cancels any animation currently in progress. */
    virtual void SetText(FText InText) override;

    /** Builds one frame while retaining hidden line breaks and character spacing. */
    static FString FormatFrame(const FString &FullText, int32 RevealedCharacterCount, const FString &CaretSlot);

protected:
    /** Clears world timers before Slate releases the widget. */
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    /** Starts character reveals after the configured initial delay. */
    void BeginTyping();
    /** Reveals the next source character and transitions to caret blinking. */
    void RevealNextCharacter();
    /** Toggles the caret after all source characters are visible. */
    void ToggleCaret();
    /** Pushes the current animation frame into the underlying Slate text block. */
    void UpdateDisplayedText();
    /** Cancels every timer owned by this widget. */
    void ClearAnimationTimers();

    UPROPERTY(Transient)
    FText TargetText;

    FTimerHandle StartTimerHandle;
    FTimerHandle CharacterTimerHandle;
    FTimerHandle CaretTimerHandle;
    int32 RevealedCharacterCount = 0;
    bool bHasTargetText = false;
    bool bCaretVisible = false;
};