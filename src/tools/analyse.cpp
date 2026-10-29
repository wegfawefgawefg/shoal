// shoal analyse: the engine's opinion of every turn start of a game, and what each turn
// actually played cost its side
#include "search/analysis.hpp"
#include "search/eval.hpp"
#include "tools/commands.hpp"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace shoal {

namespace {

struct AnalyseOptions {
    double seconds = 1.0;
    int threads = 4;
    std::string out_path;
};

bool ParseAnalyseOptions(int argc, char** argv, AnalyseOptions* options) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        bool has_value = i + 1 < argc;
        if (arg == "--time" && has_value) {
            options->seconds = std::atof(argv[++i]);
        } else if (arg == "--threads" && has_value) {
            options->threads = std::max(1, std::atoi(argv[++i]));
        } else if (arg == "--out" && has_value) {
            options->out_path = argv[++i];
        } else {
            std::fprintf(stderr, "unknown option %s\n", arg.c_str());
            return false;
        }
    }
    return options->seconds > 0;
}

// one turn start and what the engine made of it
struct TurnStart {
    int ply = 0;
    int next_ply = -1; // the next turn start, or the last ply when the game ended mid-turn
    PositionAnalysis engine;
};

std::vector<TurnStart> FindTurnStarts(const GameRecord& game,
                                      const std::vector<Position>& positions) {
    std::vector<TurnStart> starts;
    int last = static_cast<int>(game.moves.size());
    for (int ply = 0; ply <= last; ply++) {
        bool start =
            ply == 0 || game.moves[static_cast<std::size_t>(ply - 1)].kind == MoveKind::EndTurn;
        if (start) {
            TurnStart turn;
            turn.ply = ply;
            starts.push_back(turn);
        }
    }
    for (std::size_t i = 0; i + 1 < starts.size(); i++) {
        starts[i].next_ply = starts[i + 1].ply;
    }
    // a turn cut short by the end of the game is still judged, by its result
    TurnStart& final_turn = starts.back();
    if (final_turn.ply < last && positions.back().result != Result::Ongoing) {
        final_turn.next_ply = last;
    }
    return starts;
}

// every turn start through a pool of threads, one planner each
void AnalyseAll(const GameRecord& game, const std::vector<Position>& positions,
                const AnalyseOptions& options, std::vector<TurnStart>* starts) {
    std::atomic<int> next{0};
    std::atomic<int> finished{0};
    std::mutex print_lock;
    int count = static_cast<int>(starts->size());
    auto work = [&]() {
        auto planner = std::make_unique<Planner>();
        PlannerInit(planner.get(), game.map);
        for (int i = next++; i < count; i = next++) {
            TurnStart& turn = (*starts)[static_cast<std::size_t>(i)];
            AnalysePosition(planner.get(), positions[static_cast<std::size_t>(turn.ply)],
                            options.seconds, &turn.engine);
            std::lock_guard<std::mutex> guard(print_lock);
            std::fprintf(stderr, "\ranalysed %d/%d turn starts", ++finished, count);
            std::fflush(stderr);
        }
    };
    std::vector<std::thread> threads;
    int thread_count = std::min(options.threads, count);
    for (int t = 0; t < thread_count; t++) {
        threads.emplace_back(work);
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
    std::fprintf(stderr, "\n");
}

std::vector<Move> PlayedMoves(const GameRecord& game, int from, int to) {
    std::vector<Move> moves;
    for (int ply = from; ply < to; ply++) {
        const Move& move = game.moves[static_cast<std::size_t>(ply)];
        if (move.kind != MoveKind::EndTurn) {
            moves.push_back(move);
        }
    }
    return moves;
}

// what the judgement of one played turn compared, both from the mover's view
struct Judged {
    bool known = false;
    int best = 0;
    int played = 0;
};

bool Decided(int score) {
    return score > kWinThreshold || score < -kWinThreshold;
}

std::string ScoreText(int score) {
    if (score > kWinThreshold) {
        return "won";
    }
    if (score < -kWinThreshold) {
        return "lost";
    }
    return std::to_string(score);
}

// a loss that changed the result reads as the change: "won>draw"
std::string LossText(const Judged& judged) {
    if (!judged.known) {
        return "";
    }
    if (judged.best <= judged.played) {
        return "0";
    }
    if (Decided(judged.best) || Decided(judged.played)) {
        std::string best = judged.best == 0 ? "draw" : ScoreText(judged.best);
        std::string played = judged.played == 0 ? "draw" : ScoreText(judged.played);
        return best + ">" + played;
    }
    return std::to_string(judged.best - judged.played);
}

// loss and mark for the turn played from starts[i], against the engine's best from there
void JudgeTurn(const std::vector<Position>& positions, const std::vector<TurnStart>& starts,
               std::size_t i, TurnAnalysis* out, Judged* judged) {
    const TurnStart& turn = starts[i];
    if (turn.next_ply < 0) {
        return;
    }
    const Position& start = positions[static_cast<std::size_t>(turn.ply)];
    const Position& next = positions[static_cast<std::size_t>(turn.next_ply)];
    int next_root = i + 1 < starts.size() ? starts[i + 1].engine.root_score : 0;
    int played = PlayedTurnValue(next, next_root, start.side);
    int best = turn.engine.follow_score;
    judged->known = true;
    judged->best = best;
    judged->played = played;
    // the engine's own turn played: no loss, whatever the clock let each search see
    const Position& played_end = next.result != Result::Ongoing
                                     ? next
                                     : positions[static_cast<std::size_t>(turn.next_ply - 1)];
    if (played_end.hash == turn.engine.best.end.hash) {
        judged->played = best;
        return;
    }
    out->loss = std::clamp(best - played, 0, kWinScore);
    out->mark = MarkForLoss(best, played);
}

void PrintTable(const GameRecord& game, const std::vector<Position>& positions,
                const std::vector<TurnStart>& starts, const std::vector<TurnAnalysis>& turns,
                const std::vector<Judged>& judged) {
    std::printf("turn  day  side     score      loss  mark\n");
    for (std::size_t i = 0; i < starts.size(); i++) {
        const TurnStart& start = starts[i];
        const TurnAnalysis& turn = turns[i];
        const Position& pos = positions[static_cast<std::size_t>(start.ply)];
        std::printf("%4zu  %3d  p%d   %8s  %8s  %s\n", i + 1, pos.day, pos.side,
                    ScoreText(turn.score).c_str(), LossText(judged[i]).c_str(),
                    turn.mark == TurnMark::None ? "" : TurnMarkName(turn.mark));
        if (pos.result != Result::Ongoing) {
            std::printf("      game over: %s\n", ResultTextFor(pos.result));
            continue;
        }
        std::string engine = TurnMovesText(game.map, turn.best);
        std::printf("      engine: %s\n", engine.empty() ? "(nothing)" : engine.c_str());
        int to = start.next_ply >= 0 ? start.next_ply : static_cast<int>(game.moves.size());
        std::string played = TurnMovesText(game.map, PlayedMoves(game, start.ply, to));
        if (played != engine) {
            std::printf("      played: %s\n", played.empty() ? "(nothing)" : played.c_str());
        }
    }
}

} // namespace

int RunAnalyse(int argc, char** argv) {
    if (argc < 1) {
        std::fprintf(stderr,
                     "usage: shoal analyse <game.sexp> [--time S] [--threads N] [--out f]\n");
        return 1;
    }
    AnalyseOptions options;
    if (!ParseAnalyseOptions(argc, argv, &options)) {
        return 1;
    }
    auto game = std::make_unique<GameRecord>();
    std::string error;
    if (!LoadGameFile(argv[0], game.get(), &error)) {
        std::fprintf(stderr, "%s: %s\n", argv[0], error.c_str());
        return 1;
    }
    std::vector<Position> positions(1);
    SetupStart(game->map, &positions[0]);
    for (const Move& move : game->moves) {
        positions.push_back(positions.back());
        ApplyMove(game->map, &positions.back(), move);
    }

    std::vector<TurnStart> starts = FindTurnStarts(*game, positions);
    std::fprintf(stderr, "%zu turn starts, %d threads, two %.2fs searches each\n", starts.size(),
                 options.threads, options.seconds);
    AnalyseAll(*game, positions, options, &starts);

    std::vector<TurnAnalysis> turns(starts.size());
    std::vector<Judged> judged(starts.size());
    int marks[2][4] = {};
    for (std::size_t i = 0; i < starts.size(); i++) {
        turns[i].ply = starts[i].ply;
        turns[i].score = starts[i].engine.score;
        turns[i].best = starts[i].engine.best.moves;
        JudgeTurn(positions, starts, i, &turns[i], &judged[i]);
        int side = positions[static_cast<std::size_t>(starts[i].ply)].side;
        marks[side][static_cast<int>(turns[i].mark)]++;
    }
    PrintTable(*game, positions, starts, turns, judged);
    for (int side = 0; side < 2; side++) {
        std::printf("p%d: %d inaccuracies, %d mistakes, %d blunders\n", side, marks[side][1],
                    marks[side][2], marks[side][3]);
    }

    if (!options.out_path.empty()) {
        game->analysis = turns;
        if (!WriteWholeFile(options.out_path, WriteGameText(*game))) {
            std::fprintf(stderr, "cannot write %s\n", options.out_path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", options.out_path.c_str());
    }
    return 0;
}

} // namespace shoal
