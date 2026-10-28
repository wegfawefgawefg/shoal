#include "search/eval.hpp"

namespace shoal {

namespace {

// a property is worth its income over the days left, plus its weight in the day-limit count
int PropertyValue(const Map& map, const Position& pos) {
    int days_left = map.day_limit - pos.day + 1;
    if (days_left > 6) {
        days_left = 6;
    }
    return map.funds_per_property * days_left + 1500;
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
    int score[2] = {0, 0};

    // material: what the army would cost to rebuild
    for (int i = 0; i < pos.unit_count; i++) {
        const Unit& unit = pos.units[i];
        score[unit.owner] += Spec(unit.type).cost * unit.hp / 100;
    }

    // properties, and captures under way at half their progress
    int prop_value = PropertyValue(map, pos);
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
        score[capturer] += stake * done / kCapturePoints / 2;
    }

    return score[side] - score[side ^ 1];
}

} // namespace shoal
