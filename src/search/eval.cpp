#include "search/eval.hpp"

namespace shoal {

namespace {

// a property is worth its income over the days left, plus its weight in the day-limit count
int PropertyValue(const Map& map, const Position& pos, const EvalWeights& weights) {
    int days_left = map.day_limit - pos.day + 1;
    if (days_left > weights.property_days) {
        days_left = weights.property_days;
    }
    return map.funds_per_property * days_left + weights.property_base;
}

} // namespace

int ResultScore(Result result, int side) {
    if (result == Result::Draw || result == Result::Ongoing) {
        return 0;
    }
    bool zero_won = result == Result::Win0;
    return (zero_won == (side == 0)) ? kWinScore : -kWinScore;
}

int Evaluate(const Map& map, const Position& pos, int side) {
    return Evaluate(map, pos, side, EvalWeights{});
}

int Evaluate(const Map& map, const Position& pos, int side, const EvalWeights& weights) {
    int score[2] = {0, 0};

    // material: what the army would cost to rebuild, scaled per type
    for (int i = 0; i < pos.unit_count; i++) {
        const Unit& unit = pos.units[i];
        int worth = Spec(unit.type).cost * weights.unit_value[static_cast<int>(unit.type)] / 100;
        score[unit.owner] += worth * unit.hp / 100;
    }

    // properties, and captures under way by their progress
    int prop_value = PropertyValue(map, pos, weights);
    for (int prop = 0; prop < map.property_count; prop++) {
        int owner = pos.prop_owner[prop];
        bool hq = map.tiles[map.property_tiles[prop]] == Terrain::Hq;
        if (owner != kNeutral) {
            score[owner] += prop_value;
        }
        if (pos.capture[prop] == kCapturePoints) {
            continue;
        }
        std::uint8_t index = pos.unit_at[map.property_tiles[prop]];
        if (index == kNoUnit) {
            continue;
        }
        int capturer = pos.units[index].owner;
        int done = kCapturePoints - pos.capture[prop];
        int stake = hq ? kWinScore / 20 : prop_value;
        score[capturer] += stake * done / kCapturePoints * weights.capture_share / 100;
    }

    // funds buy units where bases build; elsewhere they only pay for repairs
    for (int owner = 0; owner < 2; owner++) {
        score[owner] += map.production ? pos.funds[owner] : pos.funds[owner] / 5;
    }

    return score[side] - score[side ^ 1];
}

} // namespace shoal
