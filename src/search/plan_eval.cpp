#include "search/plan_eval.hpp"

#include "search/eval.hpp"

#include <cstring>

namespace shoal {

namespace {

constexpr std::uint8_t kFar = 255;
constexpr std::int32_t kUnfilled = -2147483647;

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
    return Clamp0(ctx.weights.capture_reach - best_turns * ctx.weights.capture_per_turn);
}

// fighters want enemies within reach that they hurt; each enemy is worth the funds a full
// strength hit on it would take, more the fewer turns away it is
int FighterPlacement(const PlanContext& ctx, const Position& pos, const Unit& unit) {
    const Map& map = *ctx.map;
    const UnitSpec& spec = Spec(unit.type);
    int best = 0;
    for (int i = 0; i < pos.unit_count; i++) {
        const Unit& enemy = pos.units[i];
        if (enemy.owner == ctx.side) {
            continue;
        }
        int damage = PrimaryDamage(unit.type, enemy.type);
        int secondary = SecondaryDamage(unit.type, enemy.type);
        if (secondary > damage) {
            damage = secondary;
        }
        if (damage == 0) {
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
        int hit = ctx.weights.fight_damage ? damage * Spec(enemy.type).cost / 100 : spec.cost;
        int value = Clamp0(ctx.weights.fight_turns - turns) * hit;
        if (value > best) {
            best = value;
        }
    }
    return best * ctx.weights.fight_share / 1000;
}

struct WeightName {
    const char* name;
    int PlanWeights::*field;
};

const WeightName kWeightNames[] = {
    {"exposure", &PlanWeights::exposure},
    {"second_hit", &PlanWeights::second_hit},
    {"capture_reach", &PlanWeights::capture_reach},
    {"capture_per_turn", &PlanWeights::capture_per_turn},
    {"fight_turns", &PlanWeights::fight_turns},
    {"fight_share", &PlanWeights::fight_share},
    {"base_block", &PlanWeights::base_block},
    {"idle_funds", &PlanWeights::idle_funds},
    {"fight_damage", &PlanWeights::fight_damage},
};

struct EvalWeightName {
    const char* name;
    int* (*field)(PlanWeights*);
};

int* PropertyBase(PlanWeights* w) {
    return &w->eval.property_base;
}
int* PropertyDays(PlanWeights* w) {
    return &w->eval.property_days;
}
int* CaptureShare(PlanWeights* w) {
    return &w->eval.capture_share;
}

const EvalWeightName kEvalWeightNames[] = {
    {"property_base", PropertyBase},
    {"property_days", PropertyDays},
    {"capture_share", CaptureShare},
};

} // namespace

int* PlanWeightField(PlanWeights* weights, const std::string& name) {
    for (const WeightName& entry : kWeightNames) {
        if (name == entry.name) {
            return &(weights->*entry.field);
        }
    }
    for (const EvalWeightName& entry : kEvalWeightNames) {
        if (name == entry.name) {
            return entry.field(weights);
        }
    }
    // value_<unit>: worth of a unit type in percent of its cost
    if (name.rfind("value_", 0) == 0) {
        UnitType type;
        if (UnitTypeFromShortName(name.substr(6), &type)) {
            return &weights->eval.unit_value[static_cast<int>(type)];
        }
    }
    return nullptr;
}

bool SetPlanWeight(PlanWeights* weights, const std::string& name, int value) {
    int* field = PlanWeightField(weights, name);
    if (field == nullptr) {
        return false;
    }
    *field = value;
    return true;
}

std::string PlanWeightsText(const PlanWeights& weights) {
    std::string text;
    for (const WeightName& entry : kWeightNames) {
        if (!text.empty()) {
            text += " ";
        }
        text += std::string(entry.name) + "=" + std::to_string(weights.*entry.field);
    }
    PlanWeights copy = weights;
    for (const EvalWeightName& entry : kEvalWeightNames) {
        text += " " + std::string(entry.name) + "=" + std::to_string(*entry.field(&copy));
    }
    for (int type = 0; type < kUnitTypeCount; type++) {
        if (weights.eval.unit_value[type] != 100) {
            text += " value_" + std::string(Spec(static_cast<UnitType>(type)).short_name) + "=" +
                    std::to_string(weights.eval.unit_value[type]);
        }
    }
    return text;
}

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
               const PlanWeights& weights, const Position& pos) {
    ctx->weights = weights;
    ctx->map = &map;
    ctx->distances = &distances;
    ctx->side = pos.side;
    BuildThreats(map, pos, pos.side ^ 1, &ctx->threats);
    int tile_count = map.width * map.height;
    for (int type = 0; type < kUnitTypeCount; type++) {
        for (int tile = 0; tile < tile_count; tile++) {
            ctx->capture_cache[type][tile] = kUnfilled;
            ctx->fight_cache[type][tile] = kUnfilled;
        }
    }
}

int PlanUnitTerms(const PlanContext& ctx, const Position& pos, const Unit& unit) {
    const Map& map = *ctx.map;
    int score = -Exposure(map, pos, ctx.threats, unit, unit.tile, ctx.weights.second_hit) *
                ctx.weights.exposure / 100;
    int prop = map.property_at[unit.tile];
    bool capturing = prop != kNoProperty && pos.capture[prop] != kCapturePoints;
    int type = static_cast<int>(unit.type);
    if (Spec(unit.type).can_capture && !capturing) {
        std::int32_t& capture = ctx.capture_cache[type][unit.tile];
        if (capture == kUnfilled) {
            capture = CapturerPlacement(ctx, pos, unit);
        }
        score += capture;
    }
    std::int32_t& fight = ctx.fight_cache[type][unit.tile];
    if (fight == kUnfilled) {
        fight = FighterPlacement(ctx, pos, unit);
    }
    score += fight;
    // cargo is placed where its transport stands
    for (int c = 0; c < unit.cargo_count; c++) {
        Unit rider;
        rider.type = unit.cargo[c].type;
        rider.owner = unit.owner;
        rider.hp = unit.cargo[c].hp;
        rider.tile = unit.tile;
        int rider_type = static_cast<int>(rider.type);
        if (Spec(rider.type).can_capture) {
            std::int32_t& capture = ctx.capture_cache[rider_type][unit.tile];
            if (capture == kUnfilled) {
                capture = CapturerPlacement(ctx, pos, rider);
            }
            score += capture;
        }
    }
    if (map.production && map.tiles[unit.tile] == Terrain::Base) {
        score -= ctx.weights.base_block; // standing on our base blocks next turn's build
    }
    return score;
}

int PlanScore(const PlanContext& ctx, const Position& pos) {
    const Map& map = *ctx.map;
    if (pos.result != Result::Ongoing) {
        return ResultScore(pos.result, ctx.side);
    }
    int score = Evaluate(map, pos, ctx.side, ctx.weights.eval);
    if (map.production) {
        // unspent funds are worth a little less than the units they could buy
        score -= pos.funds[ctx.side] * ctx.weights.idle_funds / 100;
    }
    for (int i = 0; i < pos.unit_count; i++) {
        const Unit& unit = pos.units[i];
        if (unit.owner == ctx.side) {
            score += PlanUnitTerms(ctx, pos, unit);
        }
    }
    return score;
}

} // namespace shoal
