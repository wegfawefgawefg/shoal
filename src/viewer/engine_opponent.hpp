#pragma once

#include "state.hpp"

namespace shoal {

// key O in play mode: the engine takes the side not to move now, and plays its turns whenever
// the newest position is its turn; O again hands the side back
void ToggleEngineOpponent(State* state);
// starts the engine's turn when it is due, plays it when it is ready; once a frame
void PollEngineOpponent(State* state);
bool EngineOpponentThinking(const State& state);
// blocks until the engine's search is done (screenshots, quitting)
void WaitEngineOpponent(State* state);

} // namespace shoal
