#pragma once

#include "game/position.hpp"

namespace shoal {

constexpr int kWinScore = 1000000;
constexpr int kWinThreshold = kWinScore - 10000;

// the static eval's tunable numbers
struct EvalWeights {
    int property_base = 1500; // a property's worth on top of its income
    int property_days = 6;    // days of income a property is worth at most
    int capture_share = 50;   // percent of a property's worth a half-done capture earns, pro rata
    int unit_value[kUnitTypeCount] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                                      100, 100, 100, 100, 100, 100, 100, 100, 100};
};

// static score in funds from the view of `side`; positive is good for `side`
int Evaluate(const Map& map, const Position& pos, int side);
int Evaluate(const Map& map, const Position& pos, int side, const EvalWeights& weights);

// value of a finished game from the view of `side`, before distance adjustment
int ResultScore(Result result, int side);

} // namespace shoal
