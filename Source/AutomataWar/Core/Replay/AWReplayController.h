#pragma once

/**
 * @file AWReplayController.h
 * @brief Non-UObject replay playback model: re-simulates from sources+seed,
 *        provides tick navigation, snapshots, events, and instruction stepping.
 */

#include "AutomataWar/Core/AutomataRules.h"
#include "AutomataWar/Core/Sim/AutomataSimulation.h"
#include "AutomataWar/Core/Lang/AutomataCompiler.h"
#include <string>
#include <vector>

namespace Automata
{

/**
 * Replay controller that owns a full simulation run and provides
 * random-access navigation through its tick history.
 */
class FAWReplayController
{
public:
	/** Initialize by compiling sources and running sim to completion. Returns false if compile fails. */
	bool Initialize(const std::string& SourceA, const std::string& SourceB, uint64_t Seed);

	/** True if initialized successfully. */
	bool IsValid() const { return bValid_; }

	/** Total number of ticks in the match (final tick index = TotalTicks-1). */
	int32_t GetTotalTicks() const { return static_cast<int32_t>(snapshots_.size()); }

	/** Current tick position. */
	int32_t GetCurrentTick() const { return currentTick_; }

	/** Navigate to an arbitrary tick (clamped). */
	void SeekToTick(int32_t Tick);

	/** Advance one tick forward. Returns false if already at end. */
	bool StepForward();

	/** Step backward one tick. Returns false if at 0. */
	bool StepBackward();

	/** Step forward until the specified robot's instruction count changes or end is reached. */
	bool StepInstruction(int32_t RobotIdx);

	/** Get snapshot at current tick. */
	const TickSnapshot& GetCurrentSnapshot() const { return snapshots_[currentTick_]; }

	/** Get snapshot at arbitrary tick (clamped). */
	const TickSnapshot& GetSnapshotAt(int32_t Tick) const;

	/** Get all events for a specific tick. */
	std::vector<SimEvent> GetEventsForTick(int32_t Tick) const;

	/** Get all events in a tick range [From, To]. */
	std::vector<SimEvent> GetEventsInRange(int32_t FromTick, int32_t ToTick) const;

	/** Get the match result. */
	const MatchResult& GetResult() const { return result_; }

	/** Get source A. */
	const std::string& GetSourceA() const { return sourceA_; }
	/** Get source B. */
	const std::string& GetSourceB() const { return sourceB_; }

	/** Get compiled program A (for line mapping). */
	const Program& GetProgramA() const { return programA_; }
	/** Get compiled program B. */
	const Program& GetProgramB() const { return programB_; }

	/** Get all snapshots. */
	const std::vector<TickSnapshot>& GetSnapshots() const { return snapshots_; }

	/** Get all events. */
	const std::vector<SimEvent>& GetAllEvents() const { return events_; }

	/** Get the grid used for this match. */
	const std::vector<CellType>& GetGrid() const { return grid_; }

	/** Get sim config. */
	const SimConfig& GetConfig() const { return config_; }

private:
	bool bValid_ = false;
	int32_t currentTick_ = 0;
	std::string sourceA_;
	std::string sourceB_;
	Program programA_;
	Program programB_;
	SimConfig config_;
	MatchResult result_;
	std::vector<TickSnapshot> snapshots_;
	std::vector<SimEvent> events_;
	std::vector<CellType> grid_;
};

} // namespace Automata
