#pragma once

#include "game/position.hpp"
#include "search/plan_eval.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace shoal {

// a whole turn for one side: its unit actions in order, without the final end
struct TurnPlan {
    std::vector<Move> moves;
    Position end; // after the last action, before the end of the turn
    int score = 0; // PlanScore from the mover's view
};

struct PlannerLimits {
    double seconds = 1.0;
    int max_turns = 16; // turn depth: 1 = our turn then their greedy reply
    int beam_width = 12; // candidate turns per side at each node (12 beat 8 by ~100 elo)
    int narrow = 4;     // the beam loses this many candidates per turn deeper (4 beat 2)
    int threads = 1;    // more than one splits the root turns over threads
    bool quiet = false;
};

struct PlannerResult {
    TurnPlan turn;
    int score = 0; // from the mover's view after the searched turns
    int turns = 0; // deepest finished turn depth
    std::uint64_t plans = 0; // turns built
    double seconds = 0;
};

struct Planner {
    const Map* map = nullptr;
    DistanceTable distances;
    PlanWeights weights;
    int leaf_mode = 1; // 0 static, 1 greedy turn, 2 halfway
    int narrow = 4;    // copied from the limits of the running search
    std::shared_ptr<PlanContext> leaf_context = std::make_shared<PlanContext>();
    std::uint64_t plans = 0;
    std::chrono::steady_clock::time_point deadline;
    bool stopped = false;
};

void PlannerInit(Planner* planner, const Map& map);

// one side's turn, built one action at a time, each the best improvement by PlanScore
TurnPlan GreedyTurn(Planner* planner, const Position& pos);
// the `width` best distinct turns from a beam over the same steps
std::vector<TurnPlan> BeamTurns(Planner* planner, const Position& pos, int width);

PlannerResult PlanTurn(Planner* planner, const Position& pos, const PlannerLimits& limits);

} // namespace shoal
