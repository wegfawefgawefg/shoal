#pragma once

#include "state.hpp"

#include <vector>

namespace shoal {

// the file's analysis entry for exactly this ply (a turn start), or null
const TurnAnalysis* AnalysisAt(const State& state, int ply);
// the entry for the turn holding this ply
const TurnAnalysis* TurnAnalysisFor(const State& state, int ply);
bool HasAnalysis(const State& state);
// an analysis score (from the view of the side to move at its ply) from player 0's view
int AnalysisForPlayer0(const State& state, const TurnAnalysis& turn);

// what the bar shows: the result once the game is over, a live result for this position,
// the analysis of the turn holding the ply, or the file's evals
bool ShownEvalForPlayer0(const State& state, int ply, int* eval);

// the engine's best turn from the shown position (live, else the file's), null if none or
// hidden with B
const std::vector<Move>* ShownBestTurn(const State& state);

// a hand-played move at `ply` makes the analysis after it stale, and the verdict on the turn
// it was played in
void TrimAnalysis(State* state, int ply);

Color MarkColor(TurnMark mark);

} // namespace shoal
