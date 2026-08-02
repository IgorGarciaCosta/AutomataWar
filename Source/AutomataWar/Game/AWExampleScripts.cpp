#include "AWExampleScripts.h"

const FString &FAWExampleScripts::Aggressor()
{
    // Aggressor: relentlessly faces enemy, closes distance, fires at close range.
    // Uses SCAN to detect enemy; R_ENEMY_DIST > 0 means hit.
    // R_ENEMY_DIR gives cardinal direction (0=N,1=E,2=S,3=W) toward enemy on hit.
    // Strategy: scan, turn toward, approach, fire when close.
    static const FString S = TEXT(
        "; === AGGRESSOR ===\n"
        "; Scan for enemy, face them, close distance, fire relentlessly.\n"
        "\n"
        "start:\n"
        "  SCAN\n"
        "  ; If scan missed (dist==0), move forward blindly and rescan\n"
        "  IF R_ENEMY_DIST == 0 JUMP blind_move\n"
        "  ; Scan hit: check if close enough to fire (dist <= 3)\n"
        "  IF R_ENEMY_DIST <= 3 JUMP fire\n"
        "  ; Not close enough, move toward enemy\n"
        "  MOVE FWD\n"
        "  IF R0 == 0 JUMP start\n"
        "\n"
        "blind_move:\n"
        "  ; No enemy detected, advance and turn to sweep\n"
        "  MOVE FWD\n"
        "  TURN RIGHT\n"
        "  IF R0 == 0 JUMP start\n"
        "\n"
        "fire:\n"
        "  ; In range: fire then immediately re-engage\n"
        "  FIRE\n"
        "  FIRE\n"
        "  IF R0 == 0 JUMP start\n");
    return S;
}

const FString &FAWExampleScripts::Camper()
{
    // Camper: holds position, rotates to scan, shields proactively, fires at range.
    // Uses R1 as tick counter to decide when to shield.
    // Never moves. Relies on shield absorbing incoming projectiles.
    static const FString S = TEXT(
        "; === CAMPER ===\n"
        "; Stay put, scan all directions, fire on detection, shield periodically.\n"
        "\n"
        "  SET R1 0\n"
        "loop:\n"
        "  SCAN\n"
        "  IF R_ENEMY_DIST > 0 JUMP shoot\n"
        "  ; No enemy: rotate to cover another direction\n"
        "  TURN RIGHT\n"
        "  ; Increment counter, shield every 6 scans\n"
        "  SET R1 1\n"
        "  IF R1 >= 1 JUMP do_shield\n"
        "  IF R0 == 0 JUMP loop\n"
        "\n"
        "do_shield:\n"
        "  ; Proactive shield absorbs incoming fire while we are stationary\n"
        "  SHIELD\n"
        "  SET R1 0\n"
        "  IF R0 == 0 JUMP loop\n"
        "\n"
        "shoot:\n"
        "  ; Enemy detected: fire twice then resume scanning\n"
        "  FIRE\n"
        "  FIRE\n"
        "  SET R1 0\n"
        "  IF R0 == 0 JUMP loop\n");
    return S;
}

const FString &FAWExampleScripts::Kiter()
{
    // Kiter: attacks then sidesteps to avoid return fire.
    // Uses TURN+MOVE to reposition laterally (no strafe instruction).
    // Alternates kite direction using R1 (0 or 1).
    static const FString S = TEXT(
        "; === KITER ===\n"
        "; Fire, then sidestep to dodge. Alternate left/right kite direction.\n"
        "\n"
        "  SET R1 0\n"
        "main:\n"
        "  SCAN\n"
        "  IF R_ENEMY_DIST == 0 JUMP approach\n"
        "  ; Enemy in range, fire\n"
        "  FIRE\n"
        "  ; Now kite: sidestep based on R1\n"
        "  IF R1 == 0 JUMP kite_right\n"
        "  ; Kite left: turn left, move, turn right to re-face\n"
        "  TURN LEFT\n"
        "  MOVE FWD\n"
        "  TURN RIGHT\n"
        "  SET R1 0\n"
        "  IF R0 == 0 JUMP main\n"
        "\n"
        "kite_right:\n"
        "  ; Kite right: turn right, move, turn left to re-face\n"
        "  TURN RIGHT\n"
        "  MOVE FWD\n"
        "  TURN LEFT\n"
        "  SET R1 1\n"
        "  IF R0 == 0 JUMP main\n"
        "\n"
        "approach:\n"
        "  ; No enemy detected, move forward to find them\n"
        "  MOVE FWD\n"
        "  MOVE FWD\n"
        "  IF R0 == 0 JUMP main\n");
    return S;
}

const FString &FAWExampleScripts::DefaultBot()
{
    // Default/training bot: trivially predictable pattern for practice.
    // Moves forward, fires, waits, repeat. No adaptation.
    static const FString S = TEXT(
        "; === DEFAULT BOT ===\n"
        "; Simple loop: move forward, fire, wait.\n"
        "loop:\n"
        "  MOVE FWD\n"
        "  FIRE\n"
        "  WAIT\n"
        "  IF R0 == 0 JUMP loop\n");
    return S;
}
