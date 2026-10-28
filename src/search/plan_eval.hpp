#pragma once

#include "game/position.hpp"
#include "search/threat.hpp"

#include <cstdint>
#include <vector>

namespace shoal {

// terrain-only move point distances between all tiles, per move type; 255 is unreachable
struct DistanceTable {
    int tile_count = 0;
    std::vector<std::uint8_t> points; // [move_type][from][to]
};

void BuildDistances(const Map& map, DistanceTable* table);

inline int TerrainDistance(const DistanceTable& table, MoveType move_type, int from, int to) {
    std::size_t n = static_cast<std::size_t>(table.tile_count);
    return table.points[static_cast<std::size_t>(move_type) * n * n +
                        static_cast<std::size_t>(from) * n + static_cast<std::size_t>(to)];
}

// what the planner needs to score positions during one side's turn
struct PlanContext {
    const Map* map = nullptr;
    const DistanceTable* distances = nullptr;
    int side = 0;
    ThreatMap threats; // the other side's reach, taken at the start of this turn
};

void PlanBegin(PlanContext* ctx, const Map& map, const DistanceTable& distances,
               const Position& pos);

// score from ctx->side's view: material, properties, funds, minus what the enemy can take
// back next turn, plus how well placed our units are
int PlanScore(const PlanContext& ctx, const Position& pos);

} // namespace shoal
