#pragma once

#include "rules/units.hpp"

#include <cstdint>
#include <string_view>

namespace shoal {

enum class Terrain : std::uint8_t {
    Plain,
    Wood,
    Mountain,
    Road,
    Bridge,
    River,
    Sea,
    Shoal,
    Reef,
    City,
    Base,
    Hq,
    Airport,
    Port,
    Count,
};

constexpr int kTerrainCount = static_cast<int>(Terrain::Count);

struct TerrainSpec {
    std::string_view name;
    char glyph = '.'; // map file character
    int defense = 0;  // stars
    bool property = false;
};

// AWBW terrain chart (awbw.amarriner.com/terrain.php), clear weather
inline constexpr TerrainSpec kTerrainSpecs[kTerrainCount] = {
    {"Plain", '.', 1, false},   {"Wood", 'w', 2, false},     {"Mountain", '^', 4, false},
    {"Road", '=', 0, false},    {"Bridge", '#', 0, false},   {"River", '~', 0, false},
    {"Sea", 's', 0, false},     {"Shoal", '_', 0, false},    {"Reef", 'r', 1, false},
    {"City", 'c', 3, true},     {"Base", 'b', 3, true},      {"HQ", 'h', 4, true},
    {"Airport", 'a', 3, true},  {"Port", 'p', 3, true},
};

// columns: foot boot treads tires
inline constexpr std::uint8_t kMoveCost[kTerrainCount][static_cast<int>(MoveType::Count)] = {
    {1, 1, 1, 2}, // plain
    {1, 1, 2, 3}, // wood
    {2, 1, 0, 0}, // mountain
    {1, 1, 1, 1}, // road
    {1, 1, 1, 1}, // bridge
    {2, 1, 0, 0}, // river
    {0, 0, 0, 0}, // sea
    {1, 1, 1, 1}, // shoal
    {0, 0, 0, 0}, // reef
    {1, 1, 1, 1}, // city
    {1, 1, 1, 1}, // base
    {1, 1, 1, 1}, // hq
    {1, 1, 1, 1}, // airport
    {1, 1, 1, 1}, // port
};

inline const TerrainSpec& Spec(Terrain terrain) {
    return kTerrainSpecs[static_cast<int>(terrain)];
}

// movement points to enter; 0 means impassable
inline int MoveCost(Terrain terrain, MoveType move_type) {
    return kMoveCost[static_cast<int>(terrain)][static_cast<int>(move_type)];
}

// own city, base and hq repair and resupply ground units
bool RepairsGround(Terrain terrain);

bool TerrainFromGlyph(char glyph, Terrain* out);

} // namespace shoal
