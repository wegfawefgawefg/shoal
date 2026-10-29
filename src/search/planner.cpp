#include "search/planner.hpp"

#include "search/eval.hpp"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <thread>

namespace shoal {

namespace {

constexpr int kMaxTurnActions = 80;
constexpr int kInfinity = kWinScore + 1;

bool Skippable(const Move& move) {
    return move.kind == MoveKind::EndTurn || (move.kind == MoveKind::Wait && move.from == move.to);
}

bool OutOfTime(Planner* planner) {
    if (!planner->stopped && std::chrono::steady_clock::now() >= planner->deadline) {
        planner->stopped = true;
    }
    return planner->stopped;
}

struct BeamNode {
    TurnPlan plan;
    bool open = true;
};

// a plain move only changes the moving unit's own terms: score it without playing it.
// Returns false when the move needs the full path (attacks, captures, builds, or leaving a
// capture half done)
bool ScoreQuietMove(const PlanContext& ctx, const TurnPlan& plan, const Move& move, int* score) {
    const Position& pos = plan.end;
    Unit moved;
    if (!QuietMoved(*ctx.map, pos, move, &moved)) {
        return false;
    }
    const Unit& unit = pos.units[pos.unit_at[move.from]];
    *score = plan.score - PlanUnitTerms(ctx, pos, unit) + PlanUnitTerms(ctx, pos, moved);
    return true;
}

// the turn's end applied: what the next side starts from
Position AfterEnd(const Map& map, const Position& end) {
    Position next = end;
    ApplyMove(map, &next, Move{});
    return next;
}

// leaf value for the side to move: how good its own greedy turn would be (leaf_mode 1), the
// position as it stands (0), or halfway between (2)
int Leaf(Planner* planner, const Position& pos) {
    if (pos.result != Result::Ongoing) {
        return ResultScore(pos.result, pos.side);
    }
    if (planner->leaf_mode == 1) {
        return GreedyTurn(planner, pos).score;
    }
    PlanContext& ctx = *planner->leaf_context;
    PlanBegin(&ctx, *planner->map, planner->distances, planner->weights, pos);
    int now = PlanScore(ctx, pos);
    if (planner->leaf_mode == 0) {
        return now;
    }
    return (now + GreedyTurn(planner, pos).score) / 2;
}

int TurnNegamax(Planner* planner, const Position& pos, int turns, int width, int alpha, int beta) {
    if (pos.result != Result::Ongoing) {
        return ResultScore(pos.result, pos.side);
    }
    if (turns == 0) {
        return Leaf(planner, pos);
    }
    std::vector<TurnPlan> plans = BeamTurns(planner, pos, width);
    int best = -kInfinity;
    for (const TurnPlan& plan : plans) {
        if (OutOfTime(planner)) {
            break;
        }
        int score;
        if (plan.end.result != Result::Ongoing) {
            score = ResultScore(plan.end.result, pos.side);
        } else {
            Position next = AfterEnd(*planner->map, plan.end);
            if (next.result != Result::Ongoing) {
                score = ResultScore(next.result, pos.side);
            } else {
                int narrower = width > 2 ? width - 2 : 2;
                score = -TurnNegamax(planner, next, turns - 1, narrower, -beta, -alpha);
            }
        }
        if (score > best) {
            best = score;
        }
        if (score > alpha) {
            alpha = score;
        }
        if (alpha >= beta) {
            break;
        }
    }
    return best;
}

} // namespace

void PlannerInit(Planner* planner, const Map& map) {
    planner->map = &map;
    BuildDistances(map, &planner->distances);
}

TurnPlan GreedyTurn(Planner* planner, const Position& pos) {
    std::vector<TurnPlan> plans = BeamTurns(planner, pos, 1);
    return plans.front();
}

std::vector<TurnPlan> BeamTurns(Planner* planner, const Position& pos, int width) {
    const Map& map = *planner->map;
    // for_overwrite: no zeroing of the big caches, PlanBegin clears what gets used
    auto ctx = std::make_unique_for_overwrite<PlanContext>();
    PlanBegin(ctx.get(), map, planner->distances, planner->weights, pos);
    auto list = std::make_unique<MoveList>();

    std::vector<BeamNode> beam(1);
    beam[0].plan.end = pos;
    beam[0].plan.score = PlanScore(*ctx, pos);
    std::vector<TurnPlan> finished;

    struct Child {
        int score;
        int parent;
        Move move;
    };
    std::vector<Child> children;
    for (int step = 0; step < kMaxTurnActions && !beam.empty(); step++) {
        children.clear();
        for (int b = 0; b < static_cast<int>(beam.size()); b++) {
            const TurnPlan& plan = beam[static_cast<std::size_t>(b)].plan;
            if (plan.end.result != Result::Ongoing) {
                continue;
            }
            GenerateMoves(map, plan.end, list.get());
            for (int i = 0; i < list->count; i++) {
                const Move& move = list->moves[i];
                if (Skippable(move)) {
                    continue;
                }
                int score;
                if (!ScoreQuietMove(*ctx, plan, move, &score)) {
                    Position child = plan.end;
                    ApplyMove(map, &child, move);
                    score = PlanScore(*ctx, child);
                }
                if (score <= plan.score) {
                    continue; // only improvements extend a turn
                }
                children.push_back({score, b, move});
            }
        }
        // every beam node is also a finished turn as it stands
        for (BeamNode& node : beam) {
            finished.push_back(node.plan);
        }
        std::sort(children.begin(), children.end(),
                  [](const Child& a, const Child& b) { return a.score > b.score; });
        std::vector<BeamNode> next;
        std::vector<std::uint64_t> seen;
        for (const Child& child : children) {
            if (static_cast<int>(next.size()) >= width) {
                break;
            }
            BeamNode node;
            node.plan = beam[static_cast<std::size_t>(child.parent)].plan;
            ApplyMove(map, &node.plan.end, child.move);
            if (std::find(seen.begin(), seen.end(), node.plan.end.hash) != seen.end()) {
                continue; // the same position by another order
            }
            seen.push_back(node.plan.end.hash);
            node.plan.moves.push_back(child.move);
            node.plan.score = child.score;
            next.push_back(std::move(node));
        }
        beam = std::move(next);
    }
    for (BeamNode& node : beam) {
        finished.push_back(node.plan);
    }
    planner->plans += finished.size();

    // best distinct finished turns
    std::sort(finished.begin(), finished.end(),
              [](const TurnPlan& a, const TurnPlan& b) { return a.score > b.score; });
    std::vector<TurnPlan> out;
    std::vector<std::uint64_t> seen;
    for (TurnPlan& plan : finished) {
        if (static_cast<int>(out.size()) >= width) {
            break;
        }
        if (std::find(seen.begin(), seen.end(), plan.end.hash) != seen.end()) {
            continue;
        }
        seen.push_back(plan.end.hash);
        out.push_back(std::move(plan));
    }
    return out;
}

namespace {

// score of one root turn at a depth, from the mover's view, with a full window
int RootScore(Planner* planner, const Position& pos, const TurnPlan& plan, int turns, int width) {
    Position next = AfterEnd(*planner->map, plan.end);
    if (plan.end.result != Result::Ongoing || next.result != Result::Ongoing) {
        Result r = plan.end.result != Result::Ongoing ? plan.end.result : next.result;
        return ResultScore(r, pos.side);
    }
    return -TurnNegamax(planner, next, turns - 1, width, -kInfinity, kInfinity);
}

// each thread deepens its share of the root turns; the answer is the best root at the deepest
// depth every root finished
PlannerResult PlanTurnThreaded(Planner* planner, const Position& pos, const PlannerLimits& limits,
                               std::chrono::steady_clock::time_point start) {
    PlannerResult result;
    std::vector<TurnPlan> roots = BeamTurns(planner, pos, limits.beam_width);
    int root_count = static_cast<int>(roots.size());
    int max_steps = (limits.max_turns + 1) / 2;
    // scores[root][step], step k is depth 2k+1; kInfinity+1 marks unfinished
    std::vector<std::vector<int>> scores(
        static_cast<std::size_t>(root_count),
        std::vector<int>(static_cast<std::size_t>(max_steps), kInfinity + 1));
    std::vector<std::uint64_t> plans(static_cast<std::size_t>(limits.threads), 0);
    int width = limits.beam_width > 2 ? limits.beam_width - 2 : 2;
    auto worker = [&](int thread) {
        Planner local = *planner;
        local.leaf_context = std::make_shared<PlanContext>(); // never shared between threads
        local.plans = 0;
        local.stopped = false;
        for (int step = 0; step < max_steps; step++) {
            for (int i = thread; i < root_count; i += limits.threads) {
                int score =
                    RootScore(&local, pos, roots[static_cast<std::size_t>(i)], 2 * step + 1, width);
                if (local.stopped) {
                    plans[static_cast<std::size_t>(thread)] = local.plans;
                    return;
                }
                scores[static_cast<std::size_t>(i)][static_cast<std::size_t>(step)] = score;
            }
        }
        plans[static_cast<std::size_t>(thread)] = local.plans;
    };
    std::vector<std::thread> pool;
    for (int t = 0; t < limits.threads; t++) {
        pool.emplace_back(worker, t);
    }
    for (std::thread& thread : pool) {
        thread.join();
    }

    result.turn = roots.front();
    result.score = roots.front().score;
    for (int step = 0; step < max_steps; step++) {
        bool all = true;
        for (int i = 0; i < root_count; i++) {
            all = all &&
                  scores[static_cast<std::size_t>(i)][static_cast<std::size_t>(step)] <= kInfinity;
        }
        if (!all) {
            break;
        }
        int best_index = 0;
        for (int i = 1; i < root_count; i++) {
            if (scores[static_cast<std::size_t>(i)][static_cast<std::size_t>(step)] >
                scores[static_cast<std::size_t>(best_index)][static_cast<std::size_t>(step)]) {
                best_index = i;
            }
        }
        result.turn = roots[static_cast<std::size_t>(best_index)];
        result.score = scores[static_cast<std::size_t>(best_index)][static_cast<std::size_t>(step)];
        result.turns = 2 * step + 1;
    }
    for (std::uint64_t count : plans) {
        result.plans += count;
    }
    result.seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    if (!limits.quiet) {
        std::printf("threads %d turns %d score %d plans %llu time %.2fs\n", limits.threads,
                    result.turns, result.score, static_cast<unsigned long long>(result.plans),
                    result.seconds);
    }
    return result;
}

} // namespace

PlannerResult PlanTurn(Planner* planner, const Position& pos, const PlannerLimits& limits) {
    PlannerResult result;
    auto start = std::chrono::steady_clock::now();
    planner->plans = 0;
    planner->stopped = false;
    planner->deadline = start + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                    std::chrono::duration<double>(limits.seconds));
    if (limits.threads > 1) {
        return PlanTurnThreaded(planner, pos, limits, start);
    }

    std::vector<TurnPlan> roots = BeamTurns(planner, pos, limits.beam_width);
    result.turn = roots.front();
    result.score = roots.front().score;
    // odd depths only, so the last turn searched is always the reply to ours
    for (int turns = 1; turns <= limits.max_turns; turns += 2) {
        int best = -kInfinity;
        int best_index = 0;
        for (int i = 0; i < static_cast<int>(roots.size()); i++) {
            const TurnPlan& plan = roots[static_cast<std::size_t>(i)];
            int score;
            Position next = AfterEnd(*planner->map, plan.end);
            if (plan.end.result != Result::Ongoing || next.result != Result::Ongoing) {
                Result r = plan.end.result != Result::Ongoing ? plan.end.result : next.result;
                score = ResultScore(r, pos.side);
            } else {
                int width = limits.beam_width > 2 ? limits.beam_width - 2 : 2;
                score = -TurnNegamax(planner, next, turns - 1, width, -kInfinity, -best);
            }
            if (planner->stopped) {
                break;
            }
            if (score > best) {
                best = score;
                best_index = i;
            }
        }
        if (planner->stopped) {
            break;
        }
        result.turn = roots[static_cast<std::size_t>(best_index)];
        result.score = best;
        result.turns = turns;
        double seconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        if (!limits.quiet) {
            std::printf("turns %d score %d plans %llu time %.2fs\n", turns, best,
                        static_cast<unsigned long long>(planner->plans), seconds);
        }
        if (best > kWinThreshold || best < -kWinThreshold) {
            break;
        }
    }
    result.plans = planner->plans;
    result.seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    return result;
}

} // namespace shoal
