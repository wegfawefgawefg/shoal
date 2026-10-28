#pragma once

#include "state.hpp"

#include <string>

namespace shoal {

// a map file starts an empty game; a game file brings its moves and evals
bool LoadRecord(State* state, const std::string& path, std::string* error);

// replays game.moves from the start into state->positions and rebuilds the move rows
void RebuildPositions(State* state);

// moves to a ply (clamped), refreshing the last-move report and dropping play selections
void SetPly(State* state, int ply);

int LastPly(const State& state);
// the ply where the turn holding `ply` began; turns begin at 0 and after each "end"
int TurnStartPly(const State& state, int ply);
int NextTurnPly(const State& state, int ply);
int PrevTurnPly(const State& state, int ply);

// the engine's score after the move into `ply`, in funds, from player 0's view
bool EvalForPlayer0(const State& state, int ply, int* eval);
bool HasEvals(const State& state);

// replaces the moves from the current ply on with one new move (play mode)
void PlayMove(State* state, const Move& move);

// cheapest walk (then fewest turns) a unit could have taken for a move
std::vector<int> MovePath(const Map& map, const Position& before, const Move& move);

const char* ResultText(Result result);

} // namespace shoal
