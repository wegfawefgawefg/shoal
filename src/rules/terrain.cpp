#include "rules/terrain.hpp"

namespace shoal {

namespace {

// AWBW terrain chart (awbw.amarriner.com/terrain.php), clear weather
const TerrainSpec kSpecs[kTerrainCount] = {
    {"Plain", '.', 1, false},   {"Wood", 'w', 2, false},     {"Mountain", '^', 4, false},
    {"Road", '=', 0, false},    {"Bridge", '#', 0, false},   {"River", '~', 0, false},
    {"Sea", 's', 0, false},     {"Shoal", '_', 0, false},    {"Reef", 'r', 1, false},
    {"City", 'c', 3, true},     {"Base", 'b', 3, true},      {"HQ", 'h', 4, true},
    {"Airport", 'a', 3, true},  {"Port", 'p', 3, true},
};

// columns: foot boot treads tires
const std::uint8_t kMoveCost[kTerrainCount][static_cast<int>(MoveType::Count)] = {
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

} // namespace

const TerrainSpec& Spec(Terrain terrain) {
    return kSpecs[static_cast<int>(terrain)];
}

int MoveCost(Terrain terrain, MoveType move_type) {
    return kMoveCost[static_cast<int>(terrain)][static_cast<int>(move_type)];
}

bool RepairsGround(Terrain terrain) {
    return terrain == Terrain::City || terrain == Terrain::Base || terrain == Terrain::Hq;
}

bool TerrainFromGlyph(char glyph, Terrain* out) {
    for (int i = 0; i < kTerrainCount; i++) {
        if (kSpecs[i].glyph == glyph) {
            *out = static_cast<Terrain>(i);
            return true;
        }
    }
    return false;
}

} // namespace shoal
