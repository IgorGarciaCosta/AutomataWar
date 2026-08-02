#include "AWExampleScripts.h"

const FString& FAWExampleScripts::Aggressor()
{
	// Aggressor bot: relentlessly pursues and fires.
	// R_ENEMY_DIST tells distance to opponent.
	// R_ENEMY_DIR gives relative direction (0=ahead,1=right,2=behind,3=left).
	// IF syntax: IF Rx cmp Ry/imm label (no GOTO keyword).
	static const FString S = TEXT(
		"; === AGGRESSOR ===\n"
		"; Strategy: always face enemy, close distance, fire relentlessly.\n"
		"\n"
		"start: SCAN\n"
		"  IF R_ENEMY_DIR == 0 attack\n"
		"  IF R_ENEMY_DIR == 1 turn_r\n"
		"  IF R_ENEMY_DIR == 2 turn_r\n"
		"  TURN -1\n"
		"  IF R0 == R0 attack\n"
		"turn_r: TURN 1\n"
		"attack: IF R_ENEMY_DIST <= 3 fire\n"
		"  MOVE\n"
		"  IF R0 == R0 start\n"
		"fire: FIRE\n"
		"  MOVE\n"
		"  IF R0 == R0 start\n"
	);
	return S;
}

const FString& FAWExampleScripts::Camper()
{
	// Camper bot: holds position, shields proactively, fires from range.
	// Uses R1 as a counter (incremented via SET R1, imm after manual tracking).
	static const FString S = TEXT(
		"; === CAMPER ===\n"
		"; Strategy: never move, scan continuously, fire at spotted enemies,\n"
		"; shield periodically to absorb incoming projectiles.\n"
		"\n"
		"  SET R1, 0\n"
		"loop: SCAN\n"
		"  IF R0 > 0 shoot\n"
		"  IF R1 >= 5 do_shield\n"
		"  SET R1, 1\n"
		"  WAIT\n"
		"  IF R0 == R0 loop\n"
		"do_shield: SHIELD\n"
		"  SET R1, 0\n"
		"  IF R0 == R0 loop\n"
		"shoot: FIRE\n"
		"  FIRE\n"
		"  SET R1, 0\n"
		"  IF R0 == R0 loop\n"
	);
	return S;
}

const FString& FAWExampleScripts::Kiter()
{
	// Kiter bot: attacks then repositions to maintain distance.
	// Without STRAFE, uses TURN+MOVE+TURN to sidestep.
	static const FString S = TEXT(
		"; === KITER ===\n"
		"; Strategy: fire, then sidestep to avoid return fire.\n"
		"\n"
		"  SET R1, 0\n"
		"main: SCAN\n"
		"  IF R_ENEMY_DIST > 6 approach\n"
		"  IF R_ENEMY_DIST < 2 retreat\n"
		"  FIRE\n"
		"  IF R0 == R0 kite\n"
		"approach: MOVE\n"
		"  IF R0 == R0 main\n"
		"retreat: TURN 1\n"
		"  TURN 1\n"
		"  MOVE\n"
		"  TURN 1\n"
		"  TURN 1\n"
		"  IF R0 == R0 main\n"
		"kite: IF R1 == 0 kite_r\n"
		"  TURN -1\n"
		"  MOVE\n"
		"  TURN 1\n"
		"  SET R1, 0\n"
		"  IF R0 == R0 main\n"
		"kite_r: TURN 1\n"
		"  MOVE\n"
		"  TURN -1\n"
		"  SET R1, 1\n"
		"  IF R0 == R0 main\n"
	);
	return S;
}

const FString& FAWExampleScripts::DefaultBot()
{
	// Default/training bot: trivially predictable for practice.
	static const FString S = TEXT(
		"; === DEFAULT BOT ===\n"
		"loop: MOVE\n"
		"  FIRE\n"
		"  WAIT\n"
		"  IF R0 == R0 loop\n"
	);
	return S;
}
