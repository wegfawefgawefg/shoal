#pragma once

#include "state.hpp"

namespace shoal {

// the engine's verdict on the shown turn, its best turn, or the live analysis
void DrawEnginePanel(State* state);
// player 0's eval over the game: turn starts from the analysis, else the file's evals
void DrawEvalGraph(State* state);
// the ply a click on the graph goes to, -1 off the graph
int GraphPlyAt(const State& state, float x, float y);

} // namespace shoal
