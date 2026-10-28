#include "search/plan_eval.hpp"

#include "search/eval.hpp"

#include <cstring>

namespace shoal {

namespace {

constexpr std::uint8_t kFar = 255;

void DistancesFrom(const Map& map, MoveType move_type, int source, std::uint8_t* out) {
    int tile_count = map.width * map.height;
    std::memset(out, kFar, static_cast<std::size_t>(tile_count));
    // plain dijkstra with a bucket per distance; costs are 1..3
    static thread_local std::vector<int> buckets[256];
    for (auto& bucket : buckets) {
        bucket.clear();
    }
    out[source] = 0;
    buckets[0].push_back(source);
    for (int d = 0; d < 255; d++) {
        for (std::size_t i = 0; i < buckets[d].size(); i++) {
            int tile = buckets[d][i];
            if (out[tile] != d) {
                continue;
            }
            int around[4];
            int count = Neighbours(map, tile, around);
            for (int n = 0; n < count; n++) {
                int to = around[n];
                int cost = MoveCost(map.tiles[to], move_type);
                if (cost == 0 || d + cost >= 255 || out[to] <= d + cost) {
                    continue;
                }
                out[to] = static_cast<std::uint8_t>(d + cost);
                buckets[d + cost].push_back(to);
            }
        }
    }
}

int Clamp0(int value) {
    return value < 0 ? 0 : value;
}

// capturers want to stand on properties we do not own; worth more the closer they get
int CapturerPlacement(const PlanContext& ctx, const Position& pos, const Unit& unit) {
    const Map& map = *ctx.map;
    const UnitSpec& spec = Spec(unit.type);
    int best_turns = 99;
    for (int prop = 0; prop < map.property_count; prop++) {
        if (pos.prop_owner[prop] == ctx.side) {
            continue;
        }
        int tile = map.property_tiles[prop];
        int points = TerrainDistance(*ctx.distances, spec.move_type, unit.tile, tile);
        if (points == kFar) {
            continue;
        }
        int turns = (points + spec.move - 1) / spec.move;
        if (turns < best_turns) {
            best_turns = turns;
        }
    }
    return Clamp0(1600 - best_turns * 400);
}

// fighters want enemies they can hit within reach; worth a slice of their own cost
int FighterPlacement(const PlanContext& ctx, const Position& pos, const Unit& unit) {
    const Map& map = *ctx.map;
    const UnitSpec& spec = Spec(unit.type);
    int best_turns = 99;
    for (int i = 0; i < pos.unit_count; i++) {
        const Unit& enemy = pos.units[i];
        if (enemy.owner == ctx.side) {
            continue;
        }
        if (PrimaryDamage(unit.type, enemy.type) == 0 &&
            SecondaryDamage(unit.type, enemy.type) == 0) {
            continue;
        }
        int points = TerrainDistance(*ctx.distances, spec.move_type, unit.tile, enemy.tile);
        if (points == kFar) {
            continue;
        }
        if (spec.range_max > 1) {
            // indirects want the enemy just inside their ring, not next to them
            int gap = Distance(map, unit.tile, enemy.tile);
            if (gap >= spec.range_min && gap <= spec.range_max) {
                points = 0;
            }
        }
        int turns = (points + spec.move - 1) / spec.move;
        if (turns < best_turns) {
            best_turns = turns;
        }
    }
    return Clamp0((4 - best_turns) * spec.cost / 40);
}

} // namespace

void BuildDistances(const Map& map, DistanceTable* table) {
    int n = map.width * map.height;
    table->tile_count = n;
    std::size_t per_type = static_cast<std::size_t>(n) * static_cast<std::size_t>(n);
    table->points.assign(per_type * static_cast<std::size_t>(MoveType::Count), kFar);
    for (int type = 0; type < static_cast<int>(MoveType::Count); type++) {
        for (int from = 0; from < n; from++) {
            std::uint8_t* row = table->points.data() + static_cast<std::size_t>(type) * per_type +
                                static_cast<std::size_t>(from) * static_cast<std::size_t>(n);
            DistancesFrom(map, static_cast<MoveType>(type), from, row);
        }
    }
}

void PlanBegin(PlanContext* ctx, const Map& map, const DistanceTable& distances,
               const Position& pos) {
    ctx->map = &map;
    ctx->distances = &distances;
    ctx->side = pos.side;
    BuildThreats(map, pos, pos.side ^ 1, &ctx->threats);
}

int PlanScore(const PlanContext& ctx, const Position& pos) {
    const Map& map = *ctx.map;
    if (pos.result != Result::Ongoing) {
        return ResultScore(pos.result, ctx.side);
    }
    int score = Evaluate(map, pos, ctx.side);
    if (map.production) {
        // unspent funds are worth a little less than the units they could buy
        score -= pos.funds[ctx.side] / 6;
    }
    for (int i = 0; i < pos.unit_count; i++) {
        const Unit& unit = pos.units[i];
        if (unit.owner != ctx.side) {
            continue;
        }
        score -= Exposure(map, pos, ctx.threats, unit, unit.tile);
        bool capturing = map.property_at[unit.tile] != kNoProperty &&
                         pos.capture[map.property_at[unit.tile]] != kCapturePoints;
        if (Spec(unit.type).can_capture && !capturing) {
            score += CapturerPlacement(ctx, pos, unit);
        }
        score += FighterPlacement(ctx, pos, unit) / 2;
        if (map.production && map.tiles[unit.tile] == Terrain::Base) {
            score -= 400; // standing on our base blocks next turn's build
        }
    }
    return score;
}

} // namespace shoal
