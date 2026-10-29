#pragma once

#include "state.hpp"

#include <string_view>
#include <vector>

namespace shoal {

// what board art draws into: the board texture, origin at the board's top left
struct Canvas {
    SDL_Renderer* renderer = nullptr;
    Font* font = nullptr;
    const Map* map = nullptr;
    float tile = 0;
};

Frame TileFrame(const Canvas& canvas, int tile);
Vec2 TileCentre(const Canvas& canvas, int tile);

// a small rounded label centred on a point
void DrawChip(const Canvas& canvas, Vec2 centre, std::string_view text, Color fill, Color ink);

// renders the board (terrain, units, marks, play overlays) and its coordinate labels
void DrawBoard(State* state);

// the walked path as a band with an arrow head stopping short of the destination centre
void DrawPathArrow(const Canvas& canvas, const std::vector<int>& path, Color fill);

// last move marks, under and over the units (move_marks.cpp)
void DrawMoveTint(const Canvas& canvas, const MoveReport& report);
void DrawMovePath(const Canvas& canvas, const MoveReport& report);
void DrawMoveMarks(const Canvas& canvas, const MoveReport& report, const Position& before);

// the engine's best turn from the shown position: numbered arrows (engine_marks.cpp)
void DrawBestTurnUnder(const Canvas& canvas, const State& state);
void DrawBestTurnOver(const Canvas& canvas, const State& state);

} // namespace shoal
