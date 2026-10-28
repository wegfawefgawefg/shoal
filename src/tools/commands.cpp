#include "tools/commands.hpp"

#include "game/position.hpp"
#include "io/board_text.hpp"
#include "io/map_file.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace shoal {

namespace {

double Seconds(std::chrono::steady_clock::time_point start) {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - start).count();
}

// a map file starts the game; a game file replays to its last move
bool LoadPosition(const char* path, GameRecord* game, Position* pos) {
    std::string text;
    if (!ReadWholeFile(path, &text)) {
        std::fprintf(stderr, "cannot read %s\n", path);
        return false;
    }
    std::string error;
    bool is_game = text.find("(game") != std::string::npos;
    if (is_game) {
        if (!LoadGameText(text, game, &error)) {
            std::fprintf(stderr, "%s: %s\n", path, error.c_str());
            return false;
        }
    } else if (!LoadMapText(text, &game->map, &error)) {
        std::fprintf(stderr, "%s: %s\n", path, error.c_str());
        return false;
    }
    SetupStart(game->map, pos);
    for (const Move& move : game->moves) {
        ApplyMove(game->map, pos, move);
    }
    return true;
}

const char* ResultText(Result result) {
    switch (result) {
    case Result::Ongoing:
        return "ongoing";
    case Result::Win0:
        return "player 0 wins";
    case Result::Win1:
        return "player 1 wins";
    case Result::Draw:
        return "draw";
    }
    return "?";
}

std::uint64_t Perft(const Map& map, const Position& pos, int depth,
                    std::vector<MoveList>& lists) {
    if (depth == 0 || pos.result != Result::Ongoing) {
        return 1;
    }
    MoveList& list = lists[static_cast<std::size_t>(depth)];
    GenerateMoves(map, pos, &list);
    if (depth == 1) {
        return static_cast<std::uint64_t>(list.count);
    }
    std::uint64_t total = 0;
    for (int i = 0; i < list.count; i++) {
        Position child = pos;
        ApplyMove(map, &child, list.moves[i]);
        total += Perft(map, child, depth - 1, lists);
    }
    return total;
}

struct Rng {
    std::uint64_t state = 0x853c49e6748fea9bull;
    std::uint64_t Next() {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return state;
    }
};

} // namespace

int RunShow(int argc, char** argv) {
    if (argc < 1) {
        std::fprintf(stderr, "usage: shoal show <map|game>\n");
        return 1;
    }
    auto game = std::make_unique<GameRecord>();
    Position pos;
    if (!LoadPosition(argv[0], game.get(), &pos)) {
        return 1;
    }
    std::printf("%s", BoardText(game->map, pos).c_str());
    std::printf("%s\n", ResultText(pos.result));
    return 0;
}

int RunMoves(int argc, char** argv) {
    if (argc < 1) {
        std::fprintf(stderr, "usage: shoal moves <map|game>\n");
        return 1;
    }
    auto game = std::make_unique<GameRecord>();
    Position pos;
    if (!LoadPosition(argv[0], game.get(), &pos)) {
        return 1;
    }
    auto list = std::make_unique<MoveList>();
    GenerateMoves(game->map, pos, list.get());
    for (int i = 0; i < list->count; i++) {
        std::printf("%s\n", MoveText(game->map, list->moves[i]).c_str());
    }
    std::printf("%d moves\n", list->count);
    return 0;
}

int RunPerft(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: shoal perft <map|game> <plies>\n");
        return 1;
    }
    auto game = std::make_unique<GameRecord>();
    Position pos;
    if (!LoadPosition(argv[0], game.get(), &pos)) {
        return 1;
    }
    int max_depth = std::atoi(argv[1]);
    std::vector<MoveList> lists(static_cast<std::size_t>(max_depth) + 1);
    for (int depth = 1; depth <= max_depth; depth++) {
        auto start = std::chrono::steady_clock::now();
        std::uint64_t nodes = Perft(game->map, pos, depth, lists);
        double seconds = Seconds(start);
        std::printf("perft %d: %llu  (%.3f s, %.1f M/s)\n", depth,
                    static_cast<unsigned long long>(nodes), seconds,
                    seconds > 0 ? static_cast<double>(nodes) / seconds / 1e6 : 0.0);
    }
    return 0;
}

int RunRandom(int argc, char** argv) {
    if (argc < 1) {
        std::fprintf(stderr, "usage: shoal random <map> [games] [seed]\n");
        return 1;
    }
    auto game = std::make_unique<GameRecord>();
    Position start_pos;
    if (!LoadPosition(argv[0], game.get(), &start_pos)) {
        return 1;
    }
    int games = argc >= 2 ? std::atoi(argv[1]) : 1000;
    Rng rng;
    if (argc >= 3) {
        rng.state ^= std::strtoull(argv[2], nullptr, 10) * 0x9e3779b97f4a7c15ull;
    }
    auto list = std::make_unique<MoveList>();
    int results[4] = {};
    std::uint64_t plies = 0;
    auto start = std::chrono::steady_clock::now();
    for (int g = 0; g < games; g++) {
        Position pos = start_pos;
        while (pos.result == Result::Ongoing) {
            GenerateMoves(game->map, pos, list.get());
            const Move& move = list->moves[rng.Next() % static_cast<std::uint64_t>(list->count)];
            ApplyMove(game->map, &pos, move);
            plies++;
        }
        results[static_cast<int>(pos.result)]++;
    }
    double seconds = Seconds(start);
    std::printf("%d games: p0 %d, p1 %d, draw %d; %.1f plies/game; %.2f M plies/s\n", games,
                results[1], results[2], results[3],
                static_cast<double>(plies) / games, static_cast<double>(plies) / seconds / 1e6);
    return 0;
}

} // namespace shoal
