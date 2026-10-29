#pragma once

#include "state.hpp"

namespace shoal {

// starts the planner on the shown position on a background thread (key A); a press while
// the same position is being searched does nothing
void StartLiveAnalysis(State* state);
// joins finished searches and keeps the newest result; call once a frame
void PollLiveAnalysis(State* state);
bool LiveAnalysisRunning(const State& state);
// true while the newest search is for the shown position
bool LiveThinkingHere(const State& state);
// the live result when it was made for the shown position, else null
const LiveResult* LiveResultHere(const State& state);
// blocks until every search is done (screenshots, quitting)
void WaitLiveAnalysis(State* state);

} // namespace shoal
