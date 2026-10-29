#pragma once

#include "game/position.hpp"

namespace shoal {

constexpr int kWinScore = 1000000;
constexpr int kWinThreshold = kWinScore - 10000;

// the static eval's tunable numbers
struct EvalWeights {
    int property_base = 1031; // a property's worth on top of its income
    int property_days = 3;    // days of income a property is worth at most
    int capture_share = 63;   // percent of a property's worth a half-done capture earns, pro rata
    int endgame_days = 8;     // days before the limit when the property count starts to press
    int endgame_prop = 2500;  // per property of difference, per day inside that window
    // inf mech recon tank md neo art rkt aa msl apc, then fighter bomber bcopter tcopter
    // battleship cruiser lander sub (air and sea untuned yet)
    int unit_value[kUnitTypeCount] = {181, 103, 69,  146, 73,  100, 94,  100, 111, 100,
                                      100, 100, 100, 100, 100, 100, 100, 100, 100};
};

// static score in funds from the view of `side`; positive is good for `side`
int Evaluate(const Map& map, const Position& pos, int side);
int Evaluate(const Map& map, const Position& pos, int side, const EvalWeights& weights);

// value of a finished game from the view of `side`, before distance adjustment
int ResultScore(Result result, int side);

} // namespace shoal
