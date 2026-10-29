#include "io/board_text.hpp"
#include "search/planner.hpp"
#include "tools/commands.hpp"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

namespace shoal {

namespace {

struct PlanOptions {
    PlannerLimits limits;
    std::string out_path;
    int max_days = 0;
    bool show_turns = false;
};

bool ParsePlanOptions(int argc, char** argv, int first, PlanOptions* options) {
    for (int i = first; i < argc; i++) {
        std::string arg = argv[i];
        bool has_value = i + 1 < argc;
        if (arg == "--time" && has_value) {
            options->limits.seconds = std::atof(argv[++i]);
        } else if (arg == "--turns" && has_value) {
            options->limits.max_turns = std::atoi(argv[++i]);
        } else if (arg == "--beam" && has_value) {
            options->limits.beam_width = std::atoi(argv[++i]);
        } else if (arg == "--out" && has_value) {
            options->out_path = argv[++i];
        } else if (arg == "--days" && has_value) {
            options->max_days = std::atoi(argv[++i]);
        } else if (arg == "--threads" && has_value) {
            options->limits.threads = std::atoi(argv[++i]);
        } else if (arg == "--show") {
            options->show_turns = true;
        } else if (arg == "--quiet") {
            options->limits.quiet = true;
        } else {
            std::fprintf(stderr, "unknown option %s\n", arg.c_str());
            return false;
        }
    }
    return true;
}

std::string TurnText(const Map& map, const TurnPlan& turn) {
    std::string text;
    for (const Move& move : turn.moves) {
        if (!text.empty()) {
            text += ", ";
        }
        text += MoveText(map, move);
    }
    return text.empty() ? "(nothing)" : text;
}

} // namespace

int RunPlan(int argc, char** argv) {
    if (argc < 1) {
        std::fprintf(stderr, "usage: shoal plan <map|game> [--time S] [--turns N] [--beam W]\n");
        return 1;
    }
    PlanOptions options;
    if (!ParsePlanOptions(argc, argv, 1, &options)) {
        return 1;
    }
    auto game = std::make_unique<GameRecord>();
    Position pos;
    if (!LoadPosition(argv[0], game.get(), &pos)) {
        return 1;
    }
    std::printf("%s", BoardText(game->map, pos).c_str());
    auto planner = std::make_unique<Planner>();
    PlannerInit(planner.get(), game->map);
    PlannerResult result = PlanTurn(planner.get(), pos, options.limits);
    std::printf("turn: %s\nscore %d  turns %d  plans %llu  %.2fs\n",
                TurnText(game->map, result.turn).c_str(), result.score, result.turns,
                static_cast<unsigned long long>(result.plans), result.seconds);
    return 0;
}

int RunPlanPlay(int argc, char** argv) {
    if (argc < 1) {
        std::fprintf(stderr, "usage: shoal planplay <map> [--time S] [--turns N] [--out f]\n");
        return 1;
    }
    PlanOptions options;
    options.limits.quiet = true;
    if (!ParsePlanOptions(argc, argv, 1, &options)) {
        return 1;
    }
    auto game = std::make_unique<GameRecord>();
    Position pos;
    if (!LoadPosition(argv[0], game.get(), &pos)) {
        return 1;
    }
    auto planner = std::make_unique<Planner>();
    PlannerInit(planner.get(), game->map);
    while (pos.result == Result::Ongoing) {
        if (options.max_days > 0 && pos.day > options.max_days) {
            break;
        }
        PlannerResult result = PlanTurn(planner.get(), pos, options.limits);
        std::printf("day %2d p%d  score %7d  turns %d  plans %6llu  %s\n", pos.day, pos.side,
                    result.score, result.turns, static_cast<unsigned long long>(result.plans),
                    TurnText(game->map, result.turn).c_str());
        std::fflush(stdout);
        for (const Move& move : result.turn.moves) {
            ApplyMove(game->map, &pos, move);
            game->moves.push_back(move);
            game->evals.push_back(result.score);
        }
        if (pos.result == Result::Ongoing) {
            ApplyMove(game->map, &pos, Move{});
            game->moves.push_back(Move{});
            game->evals.push_back(result.score);
        }
        if (options.show_turns) {
            std::printf("%s", BoardText(game->map, pos).c_str());
        }
    }
    std::printf("%s", BoardText(game->map, pos).c_str());
    std::printf("%s\n", ResultTextFor(pos.result));
    if (!options.out_path.empty()) {
        if (!WriteWholeFile(options.out_path, WriteGameText(*game))) {
            std::fprintf(stderr, "cannot write %s\n", options.out_path.c_str());
            return 1;
        }
        std::printf("wrote %s\n", options.out_path.c_str());
    }
    return 0;
}

} // namespace shoal
