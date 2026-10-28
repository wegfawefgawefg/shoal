#pragma once

#include "state.hpp"

#include <string>

namespace shoal {

// day, players, funds, the last move line, the key hints, and the eval bar beside the board
void DrawSidePanel(State* state);
void DrawEvalBar(State* state);

// "+1.2k", "-350", "+won"; funds units from player 0's view
std::string FormatEval(int eval);
std::string FormatFunds(int funds);

// the move list (move_panel.cpp)
void DrawMoveList(State* state);
// the row under a point, -1 if none
int MoveRowAt(const State& state, float x, float y);
// the ply a row stands for: after its move, or before the first move of a turn heading
int RowPly(const State& state, int row);
void ScrollMoveList(State* state, float amount);

} // namespace shoal
