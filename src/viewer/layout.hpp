#pragma once

#include "state.hpp"

namespace shoal {

// fits the board and the side panel into a render area of this size
void ComputeLayout(State* state, float width, float height);

// padding and font sizes scale with the window
float Ui(const State& state, float px);

// the tile under a point in render pixels, -1 off the board
int TileAtPoint(const State& state, float x, float y);
Rect TileRect(const State& state, int tile);

} // namespace shoal
