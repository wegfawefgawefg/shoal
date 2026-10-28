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

const TerrainSpec& Spec(Terrain terrain);

// movement points to enter; 0 means impassable
int MoveCost(Terrain terrain, MoveType move_type);

// own city, base and hq repair and resupply ground units
bool RepairsGround(Terrain terrain);

bool TerrainFromGlyph(char glyph, Terrain* out);

} // namespace shoal
