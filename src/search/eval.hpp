#pragma once

#include "game/position.hpp"

namespace shoal {

constexpr int kWinScore = 1000000;
constexpr int kWinThreshold = kWinScore - 10000;

// static score in funds from the view of `side`; positive is good for `side`
int Evaluate(const Map& map, const Position& pos, int side);

// value of a finished game from the view of `side`, before distance adjustment
int ResultScore(Result result, int side);

} // namespace shoal
