#pragma once

#include "draw.hpp"
#include "game/map.hpp"

#include <SDL3/SDL.h>

namespace shoal {

// a stable 0..1 value per tile, so decoration does not shimmer between frames
float TileNoise(int tile, int salt);

// the ground of one tile; properties get their pad here and their building from DrawProperty
void DrawTerrain(SDL_Renderer* renderer, const Map& map, int tile, float x, float y, float s);

// owner is 0, 1 or kNeutral
void DrawProperty(SDL_Renderer* renderer, Terrain terrain, int owner, float x, float y, float s);

// property colours for an owner (0, 1, kNeutral)
Color OwnerColor(int owner);
Color OwnerDeep(int owner);

} // namespace shoal
