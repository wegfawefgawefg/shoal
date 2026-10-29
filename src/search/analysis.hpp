#pragma once

#include "io/map_file.hpp"
#include "search/planner.hpp"

#include <cstdint>

namespace shoal {

// funds a turn may lose against the engine's best before it is marked
constexpr int kInaccuracyLoss = 1500;
constexpr int kMistakeLoss = 4000;
constexpr int kBlunderLoss = 8000;

// what the engine makes of one position. PlanTurn's score always ends its search on the
// other side's reply, which leans against the side to move; the turn after it leans the
// other way. The engine's best turn is searched again one turn deeper (follow), and the
// position's score is the mean of the two, so turn starts of both sides compare
struct PositionAnalysis {
    TurnPlan best;        // the rest of this turn, without the final end
    int root_score = 0;   // PlanTurn from this position, mover's view
    int follow_score = 0; // the best turn then the other side's PlanTurn, mover's view
    int score = 0;        // mover's view; the two above blended
    int turns = 0;        // turn depth of the root search
    std::uint64_t plans = 0;
    double seconds = 0;
};

// two planner searches of `seconds` each: the position, then the reply to the best turn
void AnalysePosition(Planner* planner, const Position& pos, double seconds, PositionAnalysis* out);

// a game that is over scores as its result; otherwise the mean of the two searches, unless
// one of them sees a decided game
int BlendScores(int root_score, int follow_score);

// value of a played turn for its mover: the other side's root search after it, negated
// (pass the analysis of the next turn start), or the result once the game is over
int PlayedTurnValue(const Position& next_start, int next_root_score, int mover);

// loss is best_value - played_value, both from the mover's view at the same search depth
TurnMark MarkForLoss(int best_value, int played_value);

} // namespace shoal
