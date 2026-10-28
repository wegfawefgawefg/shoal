#pragma once

#include "game/position.hpp"
#include "search/threat.hpp"

#include <cstdint>
#include <string>
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

// the planner's tunable numbers; names are what engine specs use ("plan:w.exposure=80")
struct PlanWeights {
    int exposure = 100;        // percent of the expected loss to enemy fire counted
    int second_hit = 50;       // percent of the second best enemy hit added to the best
    int capture_reach = 1600;  // a capturer standing next to a property we lack
    int capture_per_turn = 400; // what each turn of walking to it costs
    int fight_turns = 4;       // fighters value enemies they can reach within this many turns
    int fight_share = 12;      // per mille of a fighter's cost, per turn closer than that
    int base_block = 400;      // our own unit on our base at the end of the turn
    int idle_funds = 17;       // percent of unspent funds subtracted where bases build
};

bool SetPlanWeight(PlanWeights* weights, const std::string& name, int value);
std::string PlanWeightsText(const PlanWeights& weights);

// what the planner needs to score positions during one side's turn
struct PlanContext {
    PlanWeights weights;
    const Map* map = nullptr;
    const DistanceTable* distances = nullptr;
    int side = 0;
    ThreatMap threats; // the other side's reach, taken at the start of this turn
    // placement values by unit type and tile, filled on first use this turn; like the
    // threat map they describe the board as the turn began
    mutable std::int32_t capture_cache[kUnitTypeCount][kMaxTiles];
    mutable std::int32_t fight_cache[kUnitTypeCount][kMaxTiles];
};

void PlanBegin(PlanContext* ctx, const Map& map, const DistanceTable& distances,
               const PlanWeights& weights, const Position& pos);

// score from ctx->side's view: material, properties, funds, minus what the enemy can take
// back next turn, plus how well placed our units are
int PlanScore(const PlanContext& ctx, const Position& pos);
// the part of PlanScore one of ctx->side's units adds, as if it stood on unit.tile
int PlanUnitTerms(const PlanContext& ctx, const Position& pos, const Unit& unit);

} // namespace shoal
