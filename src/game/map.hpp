#pragma once

#include "rules/damage.hpp"
#include "rules/terrain.hpp"
#include "rules/units.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace shoal {

constexpr int kMaxSide = 32;
constexpr int kMaxTiles = kMaxSide * kMaxSide;
constexpr int kMaxProperties = 128;
constexpr int kNoProperty = -1;
constexpr std::uint8_t kNeutral = 2;

struct StartUnit {
    UnitType type = UnitType::Infantry;
    int owner = 0;
    int tile = 0;
    int hp = 100;
};

// everything about a game that never changes while it is played
struct Map {
    std::string name;
    int width = 0;
    int height = 0;
    Terrain tiles[kMaxTiles] = {};

    int property_count = 0;
    std::uint16_t property_tiles[kMaxProperties] = {};
    std::int16_t property_at[kMaxTiles] = {}; // kNoProperty when the tile is no property
    std::uint8_t start_owner[kMaxProperties] = {};

    std::vector<StartUnit> start_units;
    int start_funds[2] = {0, 0};
    int funds_per_property = 1000;
    int day_limit = 20;      // the game is scored when player 2's turn ends on this day
    bool production = false; // bases build units
    bool buildable[kUnitTypeCount] = {};

    // orthogonal neighbours of every tile, filled by IndexProperties
    std::uint8_t neighbour_count[kMaxTiles] = {};
    std::uint16_t neighbours[kMaxTiles][4] = {};
};

inline int TileX(const Map& map, int tile) {
    return tile % map.width;
}

inline int TileY(const Map& map, int tile) {
    return tile / map.width;
}

inline int TileAt(const Map& map, int x, int y) {
    return y * map.width + x;
}

inline int Distance(const Map& map, int a, int b) {
    int dx = TileX(map, a) - TileX(map, b);
    int dy = TileY(map, a) - TileY(map, b);
    return (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
}

// fills up to four orthogonal neighbours, returns the count
inline int Neighbours(const Map& map, int tile, int out[4]) {
    int count = map.neighbour_count[tile];
    for (int i = 0; i < count; i++) {
        out[i] = map.neighbours[tile][i];
    }
    return count;
}

// after the terrain is set: rebuilds the neighbour lists and property_tiles/property_at;
// keeps start_owner per property
void IndexProperties(Map* map);

// "a1" is the top left tile; columns a..z then aa..af
std::string TileName(const Map& map, int tile);
bool ParseTileName(const Map& map, std::string_view text, int* tile);

} // namespace shoal
