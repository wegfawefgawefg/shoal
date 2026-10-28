#pragma once

#include "draw.hpp"
#include "rules/units.hpp"

#include <SDL3/SDL.h>

namespace shoal {

// a side-on silhouette in the tile at (x, y) of size s; player 0 faces right, player 1 left;
// dimmed units have acted this turn
void DrawUnit(SDL_Renderer* renderer, UnitType type, int owner, float x, float y, float s,
              bool dimmed);

} // namespace shoal
