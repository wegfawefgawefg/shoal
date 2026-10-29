#include "search/analysis.hpp"

#include "search/eval.hpp"

#include <chrono>

namespace shoal {

namespace {

bool Decisive(int score) {
    return score > kWinThreshold || score < -kWinThreshold;
}

} // namespace

int BlendScores(int root_score, int follow_score) {
    if (Decisive(root_score)) {
        return root_score;
    }
    if (Decisive(follow_score)) {
        return follow_score;
    }
    return (root_score + follow_score) / 2;
}

void AnalysePosition(Planner* planner, const Position& pos, double seconds, PositionAnalysis* out) {
    *out = PositionAnalysis{};
    if (pos.result != Result::Ongoing) {
        int score = ResultScore(pos.result, pos.side);
        out->root_score = score;
        out->follow_score = score;
        out->score = score;
        return;
    }
    auto start = std::chrono::steady_clock::now();
    PlannerLimits limits;
    limits.seconds = seconds;
    limits.quiet = true;
    PlannerResult root = PlanTurn(planner, pos, limits);
    out->best = root.turn;
    out->root_score = root.score;
    out->turns = root.turns;
    out->plans = root.plans;

    // the best turn ended: the other side's search from there, one turn deeper than the root
    Position next = root.turn.end;
    if (next.result == Result::Ongoing) {
        ApplyMove(*planner->map, &next, Move{});
    }
    if (next.result != Result::Ongoing) {
        out->follow_score = ResultScore(next.result, pos.side);
    } else {
        PlannerResult reply = PlanTurn(planner, next, limits);
        out->follow_score = -reply.score;
        out->plans += reply.plans;
    }
    out->score = BlendScores(out->root_score, out->follow_score);
    out->seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

int PlayedTurnValue(const Position& next_start, int next_root_score, int mover) {
    if (next_start.result != Result::Ongoing) {
        return ResultScore(next_start.result, mover);
    }
    return next_start.side == mover ? next_root_score : -next_root_score;
}

TurnMark MarkForLoss(int best_value, int played_value) {
    // a game that was not lost and now is lost is a blunder whatever the funds say
    if (best_value >= -kWinThreshold && played_value < -kWinThreshold) {
        return TurnMark::Blunder;
    }
    int loss = best_value - played_value;
    if (loss >= kBlunderLoss) {
        return TurnMark::Blunder;
    }
    if (loss >= kMistakeLoss) {
        return TurnMark::Mistake;
    }
    if (loss >= kInaccuracyLoss) {
        return TurnMark::Inaccuracy;
    }
    return TurnMark::None;
}

} // namespace shoal
