#include "io/board_text.hpp"
#include "search/eval.hpp"
#include "search/search.hpp"
#include "tools/commands.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

namespace shoal {

namespace {

struct SearchOptions {
    SearchLimits limits;
    int table_log2 = 22;
    std::string out_path;
    int max_plies = 4000;
    int day_limit = 0; // 0 keeps the map's own
};

bool ParseOptions(int argc, char** argv, int first, SearchOptions* options) {
    for (int i = first; i < argc; i++) {
        std::string arg = argv[i];
        bool has_value = i + 1 < argc;
        if (arg == "--solve") {
            options->limits.solve = true;
        } else if (arg == "--distance") {
            options->limits.distance = true;
        } else if (arg == "--quiet") {
            options->limits.quiet = true;
        } else if (arg == "--depth" && has_value) {
            options->limits.max_depth = std::atoi(argv[++i]);
        } else if (arg == "--time" && has_value) {
            options->limits.seconds = std::atof(argv[++i]);
        } else if (arg == "--nodes" && has_value) {
            options->limits.nodes = std::strtoull(argv[++i], nullptr, 10);
        } else if (arg == "--table" && has_value) {
            options->table_log2 = std::atoi(argv[++i]);
        } else if (arg == "--days" && has_value) {
            options->day_limit = std::atoi(argv[++i]);
        } else if (arg == "--out" && has_value) {
            options->out_path = argv[++i];
        } else if (arg == "--max-plies" && has_value) {
            options->max_plies = std::atoi(argv[++i]);
        } else {
            std::fprintf(stderr, "unknown option %s\n", arg.c_str());
            return false;
        }
    }
    return true;
}

std::string LineText(const Map& map, const std::vector<Move>& line, int max_moves) {
    std::string text;
    for (int i = 0; i < static_cast<int>(line.size()) && i < max_moves; i++) {
        if (i > 0) {
            text += ", ";
        }
        text += MoveText(map, line[static_cast<std::size_t>(i)]);
    }
    return text;
}

} // namespace

int RunSearch(int argc, char** argv) {
    if (argc < 1) {
        std::fprintf(stderr, "usage: shoal search <map|game> [--depth N] [--time S] [--solve]\n");
        return 1;
    }
    SearchOptions options;
    if (!ParseOptions(argc, argv, 1, &options)) {
        return 1;
    }
    auto game = std::make_unique<GameRecord>();
    Position pos;
    if (!LoadPosition(argv[0], game.get(), &pos)) {
        return 1;
    }
    if (options.day_limit > 0) {
        game->map.day_limit = options.day_limit;
    }
    std::printf("%s", BoardText(game->map, pos).c_str());
    auto searcher = std::make_unique<Searcher>();
    SearcherInit(searcher.get(), game->map, options.table_log2);
    SearchResult result = Search(searcher.get(), pos, options.limits);
    std::printf("best %s  score %d  depth %d  nodes %llu  %.2fs (%.2f M nodes/s)\n",
                MoveText(game->map, result.best).c_str(), result.score,
                options.limits.solve ? -1 : result.depth,
                static_cast<unsigned long long>(result.nodes), result.seconds,
                result.seconds > 0 ? static_cast<double>(result.nodes) / result.seconds / 1e6
                                   : 0.0);
    if (options.limits.solve) {
        const char* verdict = result.score > 0 ? "side to move wins"
                              : result.score < 0 ? "side to move loses"
                                                 : "draw";
        std::printf("solved: %s\n", result.complete ? verdict : "unfinished (limit hit)");
    }
    std::printf("line: %s\n", LineText(game->map, result.pv, 40).c_str());
    if (!options.out_path.empty()) {
        // the line as a game from this position: the moves played so far, then the line
        Position line_pos = pos;
        int value = result.score > 0 ? kWinScore : (result.score < 0 ? -kWinScore : 0);
        std::vector<Move> line = result.pv;
        for (int pass = 0; pass < 200; pass++) {
            for (const Move& move : line) {
                int mover = line_pos.side;
                ApplyMove(game->map, &line_pos, move);
                game->moves.push_back(move);
                // solved values are from the root mover's view; store each from its mover's
                // view
                int eval = options.limits.solve ? (mover == pos.side ? value : -value) : 0;
                game->evals.push_back(eval);
            }
            // a line can break where the table lost an entry: solve again from there
            if (!options.limits.solve || line_pos.result != Result::Ongoing) {
                break;
            }
            SearchLimits again = options.limits;
            again.quiet = true;
            line = Search(searcher.get(), line_pos, again).pv;
            if (line.empty()) {
                break;
            }
        }
        if (!game->evals.empty() && game->evals.size() != game->moves.size()) {
            game->evals.clear();
        }
        if (!WriteWholeFile(options.out_path, WriteGameText(*game))) {
            std::fprintf(stderr, "cannot write %s\n", options.out_path.c_str());
            return 1;
        }
        std::printf("wrote %s (%s)\n", options.out_path.c_str(),
                    ResultTextFor(line_pos.result));
    }
    return 0;
}

int RunSelfPlay(int argc, char** argv) {
    if (argc < 1) {
        std::fprintf(stderr, "usage: shoal selfplay <map> [--time S] [--depth N] [--out f]\n");
        return 1;
    }
    SearchOptions options;
    options.limits.quiet = true;
    if (!ParseOptions(argc, argv, 1, &options)) {
        return 1;
    }
    auto game = std::make_unique<GameRecord>();
    Position pos;
    if (!LoadPosition(argv[0], game.get(), &pos)) {
        return 1;
    }
    if (options.day_limit > 0) {
        game->map.day_limit = options.day_limit;
    }
    auto searcher = std::make_unique<Searcher>();
    SearcherInit(searcher.get(), game->map, options.table_log2);
    for (int ply = 0; ply < options.max_plies && pos.result == Result::Ongoing; ply++) {
        SearchResult result = Search(searcher.get(), pos, options.limits);
        Move move = result.best;
        if (!IsLegal(game->map, pos, move)) {
            move = Move{}; // no finished iteration: pass the turn
        }
        int mover = pos.side;
        std::printf("day %d p%d  %-18s score %7d depth %2d nodes %llu\n", pos.day, mover,
                    MoveText(game->map, move).c_str(), result.score, result.depth,
                    static_cast<unsigned long long>(result.nodes));
        std::fflush(stdout);
        ApplyMove(game->map, &pos, move);
        game->moves.push_back(move);
        game->evals.push_back(result.score);
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
