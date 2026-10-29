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

// points to stand next to `target`: through the target's tile when the mover can enter it,
// else from the best neighbour (a ship hitting the shore, a tank hitting a ship)
int ApproachPoints(const PlanContext& ctx, MoveType move_type, int from, int target) {
    int points = TerrainDistance(*ctx.distances, move_type, from, target);
    if (points != kFar) {
        return points;
    }
    int around[4];
    int count = Neighbours(*ctx.map, target, around);
    for (int n = 0; n < count; n++) {
        int near = TerrainDistance(*ctx.distances, move_type, from, around[n]);
        if (near != kFar && near + 1 < points) {
            points = near + 1;
        }
    }
    return points;
}

// a foot soldier riding an air or sea transport: turns for the transport to carry it next to
// a property we lack and for it to walk on. A lander goes by way of a shoal or port
int RiderCapture(const PlanContext& ctx, const Position& pos, const Unit& transport) {
    const Map& map = *ctx.map;
    const UnitSpec& carrier = Spec(transport.type);
    const UnitSpec& foot = Spec(UnitType::Infantry);
    int tile_count = map.width * map.height;
    int best_turns = 99;
    for (int prop = 0; prop < map.property_count; prop++) {
        if (pos.prop_owner[prop] == ctx.side) {
            continue;
        }
        int tile = map.property_tiles[prop];
        if (transport.type != UnitType::Lander) {
            int points = ApproachPoints(ctx, carrier.move_type, transport.tile, tile);
            if (points != kFar) {
                int turns = (points + carrier.move - 1) / carrier.move;
                best_turns = turns < best_turns ? turns : best_turns;
            }
            continue;
        }
        for (int land = 0; land < tile_count; land++) {
            if (map.tiles[land] != Terrain::Shoal && map.tiles[land] != Terrain::Port) {
                continue;
            }
            int sail = TerrainDistance(*ctx.distances, carrier.move_type, transport.tile, land);
            int walk = TerrainDistance(*ctx.distances, foot.move_type, land, tile);
            if (sail == kFar || walk == kFar) {
                continue;
            }
            int turns =
                (sail + carrier.move - 1) / carrier.move + (walk + foot.move - 1) / foot.move;
            best_turns = turns < best_turns ? turns : best_turns;
        }
    }
    return Clamp0(ctx.weights.capture_reach - best_turns * ctx.weights.capture_per_turn);
}

// air and sea units that cannot get back to an own airport or port with fuel to spare lose
// a share of their worth: all of fuel_risk once they would run dry on the way
int FuelRisk(const PlanContext& ctx, const Position& pos, const Unit& unit) {
    const Map& map = *ctx.map;
    const UnitSpec& spec = Spec(unit.type);
    int burn = unit.dived ? kSubDivedFuelPerDay : spec.fuel_per_day;
    if (burn == 0) {
        return 0;
    }
    int need = kFar;
    for (int prop = 0; prop < map.property_count; prop++) {
        int tile = map.property_tiles[prop];
        if (pos.prop_owner[prop] != ctx.side || !RepairsUnit(map.tiles[tile], unit.type)) {
            continue;
        }
        int points = TerrainDistance(*ctx.distances, spec.move_type, unit.tile, tile);
        need = points < need ? points : need;
    }
    int margin = unit.fuel;
    if (need != kFar) {
        int turns = (need + spec.move - 1) / spec.move;
        margin -= need + burn * turns;
    }
    constexpr int kSpareDays = 3;
    int spare_days = margin < 0 ? -1 : margin / burn;
    if (spare_days >= kSpareDays) {
        return 0;
    }
    int short_days = kSpareDays - (spare_days < 0 ? 0 : spare_days);
    return spec.cost * ctx.weights.fuel_risk / 100 * short_days / kSpareDays;
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
        if (enemy.dived && !HitsDived(unit.type)) {
            continue;
        }
        int points = ApproachPoints(ctx, spec.move_type, unit.tile, enemy.tile);
        if (points == kFar) {
            continue;
        }
        int turns = (points + spec.move - 1) / spec.move;
        if (spec.range_max > 1) {
            // indirects want the enemy inside their ring; outside it they must walk to the
            // ring and then wait a turn, since they cannot move and fire
            int gap = Distance(map, unit.tile, enemy.tile);
            if (gap >= spec.range_min && gap <= spec.range_max) {
                turns = 0;
            } else if (gap < spec.range_min) {
                turns = 1;
            } else {
                int walk = points - spec.range_max;
                turns = 1 + (walk > 0 ? (walk + spec.move - 1) / spec.move : 0);
            }
        }
        int hit = damage * Spec(enemy.type).cost / 100;
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
    {"excess_funds", &PlanWeights::excess_funds},
    {"spend_per_base", &PlanWeights::spend_per_base},
    {"fuel_risk", &PlanWeights::fuel_risk},
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
    for (int type = 0; type < kUnitTypeCount; type++) {
        ctx->cache_ready[type] = false;
    }
    for (int t = 0; t < kTerrainCount; t++) {
        ctx->blocks_build[t] = false;
        for (int type = 0; type < kUnitTypeCount && map.production; type++) {
            if (map.buildable[type] &&
                BuildsUnit(static_cast<Terrain>(t), static_cast<UnitType>(type))) {
                ctx->blocks_build[t] = true;
            }
        }
    }
}

namespace {

// clears a type's placement caches the first time this turn the type is scored
void PrepareType(const PlanContext& ctx, int type) {
    if (ctx.cache_ready[type]) {
        return;
    }
    ctx.cache_ready[type] = true;
    int tile_count = ctx.map->width * ctx.map->height;
    for (int tile = 0; tile < tile_count; tile++) {
        ctx.capture_cache[type][tile] = kUnfilled;
        ctx.fight_cache[type][tile] = kUnfilled;
        ctx.ride_cache[type][tile] = kUnfilled;
    }
}

} // namespace

int PlanUnitTerms(const PlanContext& ctx, const Position& pos, const Unit& unit) {
    const Map& map = *ctx.map;
    PrepareType(ctx, static_cast<int>(unit.type));
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
    if (Spec(unit.type).fuel_per_day > 0) {
        score -= FuelRisk(ctx, pos, unit);
    }
    // cargo is placed where its transport stands; air and sea transports carry it on
    bool carried = unit.cargo_count > 0 && DomainOf(unit.type) != Domain::Ground;
    for (int c = 0; c < unit.cargo_count; c++) {
        Unit rider;
        rider.type = unit.cargo[c].type;
        rider.owner = unit.owner;
        rider.hp = unit.cargo[c].hp;
        rider.tile = unit.tile;
        int rider_type = static_cast<int>(rider.type);
        PrepareType(ctx, rider_type);
        if (Spec(rider.type).can_capture && carried) {
            std::int32_t& ride = ctx.ride_cache[type][unit.tile];
            if (ride == kUnfilled) {
                ride = RiderCapture(ctx, pos, unit);
            }
            score += ride;
        } else if (Spec(rider.type).can_capture) {
            std::int32_t& capture = ctx.capture_cache[rider_type][unit.tile];
            if (capture == kUnfilled) {
                capture = CapturerPlacement(ctx, pos, rider);
            }
            score += capture;
        }
    }
    if (ctx.blocks_build[static_cast<int>(map.tiles[unit.tile])]) {
        score -= ctx.weights.base_block; // standing on a factory blocks next turn's build
    }
    return score;
}

bool QuietMoved(const Map& map, const Position& pos, const Move& move, Unit* moved) {
    if (move.kind != MoveKind::Wait && move.kind != MoveKind::Dive &&
        move.kind != MoveKind::Surface) {
        return false;
    }
    int from_prop = map.property_at[move.from];
    if (from_prop != kNoProperty && pos.capture[from_prop] != kCapturePoints &&
        move.to != move.from) {
        return false;
    }
    *moved = pos.units[pos.unit_at[move.from]];
    int points = MovePoints(map, pos, move);
    moved->fuel = static_cast<std::uint8_t>(moved->fuel > points ? moved->fuel - points : 0);
    moved->tile = move.to;
    moved->done = 1;
    if (move.kind != MoveKind::Wait) {
        moved->dived = move.kind == MoveKind::Dive ? 1 : 0;
    }
    return true;
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
        // past what our bases could spend next turn, money is worth much less: build slots,
        // not funds, are what limit us then
        int bases = 0;
        for (int prop = 0; prop < map.property_count; prop++) {
            bases += pos.prop_owner[prop] == ctx.side &&
                     map.tiles[map.property_tiles[prop]] == Terrain::Base;
        }
        int excess = pos.funds[ctx.side] - bases * ctx.weights.spend_per_base;
        if (excess > 0) {
            score -= excess * ctx.weights.excess_funds / 100;
        }
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
