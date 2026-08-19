#include "AWTypewriterTextBlock.h"

#include "Engine/World.h"

void UAWTypewriterTextBlock::PlayTypewriter()
{
    ClearAnimationTimers();
    if (!bHasTargetText)
    {
        TargetText = UTextBlock::GetText();
        bHasTargetText = true;
    }

    RevealedCharacterCount = 0;
    bCaretVisible = false;
    UpdateDisplayedText();

    UWorld *World = GetWorld();
    if (!World)
    {
        UTextBlock::SetText(TargetText);
        return;
    }

    if (InitialDelay <= 0.f)
    {
        BeginTyping();
        return;
    }

    World->GetTimerManager().SetTimer(StartTimerHandle, this, &UAWTypewriterTextBlock::BeginTyping,
                                      InitialDelay, false);
}

void UAWTypewriterTextBlock::SetText(FText InText)
{
    ClearAnimationTimers();
    TargetText = InText;
    bHasTargetText = true;
    UTextBlock::SetText(MoveTemp(InText));
}

FString UAWTypewriterTextBlock::FormatFrame(const FString &FullText, int32 RevealedCharacterCount,
                                            const FString &CaretSlot)
{
    const int32 ClampedCount = FMath::Clamp(RevealedCharacterCount, 0, FullText.Len());
    FString Frame = FullText.Left(ClampedCount);
    Frame.Reserve(FullText.Len() + CaretSlot.Len());
    Frame += CaretSlot;

    for (int32 Index = ClampedCount; Index < FullText.Len(); ++Index)
    {
        const TCHAR Character = FullText[Index];
        Frame.AppendChar(Character == TEXT('\n') || Character == TEXT('\r') ? Character : TEXT(' '));
    }
    return Frame;
}

void UAWTypewriterTextBlock::ReleaseSlateResources(bool bReleaseChildren)
{
    ClearAnimationTimers();
    Super::ReleaseSlateResources(bReleaseChildren);
}

void UAWTypewriterTextBlock::BeginTyping()
{
    const FString FullText = TargetText.ToString();
    bCaretVisible = true;
    if (FullText.IsEmpty())
    {
        UpdateDisplayedText();
    }
    else if (UWorld *World = GetWorld())
    {
        World->GetTimerManager().SetTimer(CharacterTimerHandle, this,
                                          &UAWTypewriterTextBlock::RevealNextCharacter,
                                          FMath::Max(SecondsPerCharacter, 0.001f), true);
        RevealNextCharacter();
        return;
    }

    if (UWorld *World = GetWorld())
    {
        World->GetTimerManager().SetTimer(CaretTimerHandle, this, &UAWTypewriterTextBlock::ToggleCaret,
                                          FMath::Max(CaretBlinkInterval, 0.01f), true);
    }
}

void UAWTypewriterTextBlock::RevealNextCharacter()
{
    const int32 TextLength = TargetText.ToString().Len();
    RevealedCharacterCount = FMath::Min(RevealedCharacterCount + 1, TextLength);
    UpdateDisplayedText();
    if (RevealedCharacterCount < TextLength)
        return;

    if (UWorld *World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CharacterTimerHandle);
        World->GetTimerManager().SetTimer(CaretTimerHandle, this, &UAWTypewriterTextBlock::ToggleCaret,
                                          FMath::Max(CaretBlinkInterval, 0.01f), true);
    }
}

void UAWTypewriterTextBlock::ToggleCaret()
{
    bCaretVisible = !bCaretVisible;
    UpdateDisplayedText();
}

void UAWTypewriterTextBlock::UpdateDisplayedText()
{
    const FString FullText = TargetText.ToString();
    const FString Caret = CaretText.ToString();
    const FString CaretSlot = bCaretVisible ? Caret : FString::ChrN(Caret.Len(), TEXT(' '));
    UTextBlock::SetText(FText::FromString(FormatFrame(FullText, RevealedCharacterCount, CaretSlot)));
}

void UAWTypewriterTextBlock::ClearAnimationTimers()
{
    if (UWorld *World = GetWorld())
    {
        FTimerManager &TimerManager = World->GetTimerManager();
        TimerManager.ClearTimer(StartTimerHandle);
        TimerManager.ClearTimer(CharacterTimerHandle);
        TimerManager.ClearTimer(CaretTimerHandle);
    }
}