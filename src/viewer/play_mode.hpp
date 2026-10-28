#pragma once

#include "board_draw.hpp"
#include "state.hpp"

namespace shoal {

// picks the unit on a tile (or opens the build menu on an own base); false if nothing to pick
bool SelectTile(State* state, int tile);
void PlayCancel(State* state);
// a left click while in play mode; true when it was used
bool PlayClick(State* state, float x, float y);
void PlayEndTurn(State* state);

// reach and targets under the units, selection marks over them
void DrawPlayUnder(const Canvas& canvas, const State& state);
void DrawPlayOver(const Canvas& canvas, const State& state);
// the action menu, on screen
void DrawPlayMenu(State* state);

} // namespace shoal
