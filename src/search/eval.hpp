#pragma once

#include "game/position.hpp"

namespace shoal {

constexpr int kWinScore = 1000000;
constexpr int kWinThreshold = kWinScore - 10000;

// the static eval's tunable numbers
struct EvalWeights {
    int property_base = 1294; // a property's worth on top of its income
    int property_days = 9;    // days of income a property is worth at most
    int capture_share = 54;   // percent of a property's worth a half-done capture earns, pro rata
    int endgame_days = 8;     // days before the limit when the property count starts to press
    int endgame_prop = 2500;  // per property of difference, per day inside that window
    // inf mech recon tank md neo art rkt aa msl apc, then fighter bomber bcopter tcopter
    // battleship cruiser lander sub (air and sea untuned yet)
    int unit_value[kUnitTypeCount] = {198, 104, 85,  139, 73,  96,  90,  81,  107, 100,
                                      90,  100, 100, 100, 100, 100, 100, 100, 100};
};

// static score in funds from the view of `side`; positive is good for `side`
int Evaluate(const Map& map, const Position& pos, int side);
int Evaluate(const Map& map, const Position& pos, int side, const EvalWeights& weights);

// value of a finished game from the view of `side`, before distance adjustment
int ResultScore(Result result, int side);

} // namespace shoal
