#pragma once

#include "game/map.hpp"

#include <cstdint>

namespace shoal {

struct MapGenOptions {
    int width = 12;
    int height = 12;
    std::uint64_t seed = 1;
    int day_limit = 20;
    int cities_per_side = 0; // 0 picks from the area
    int bases_per_side = 1;
};

// a point-symmetric production map: hq and bases in opposite corners, neutral cities, some
// woods and mountains, every property reachable by treads from both hqs
void GenerateMap(const MapGenOptions& options, Map* map);

} // namespace shoal
