#include "game/check.hpp"
#include "io/map_file.hpp"
#include "io/map_gen.hpp"
#include "tools/commands.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace shoal {

namespace {

std::uint64_t Next(std::uint64_t* state) {
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

// one random game on a fresh map; returns an empty string or what broke, with the moves
std::string FuzzGame(std::uint64_t seed, std::uint64_t* plies) {
    std::uint64_t rng = seed * 0x9e3779b97f4a7c15ull + 1;
    for (int i = 0; i < 4; i++) {
        Next(&rng);
    }
    auto map = std::make_unique<Map>();
    MapGenOptions gen;
    gen.width = gen.height = 8 + static_cast<int>(Next(&rng) % 17);
    gen.bases_per_side = 1 + static_cast<int>(Next(&rng) % 3);
    gen.water = gen.width >= 12 && (Next(&rng) & 1) != 0;
    gen.day_limit = 6 + static_cast<int>(Next(&rng) % 15);
    gen.seed = Next(&rng);
    GenerateMap(gen, map.get());
    // random funds so expensive units come into play early
    map->start_funds[0] = map->start_funds[1] = static_cast<int>(Next(&rng) % 4) * 20000;
    auto list = std::make_unique<MoveList>();
    std::vector<Move> moves;
    Position pos;
    SetupStart(*map, &pos);
    std::string why;
    for (int ply = 0; ply < 20000 && pos.result == Result::Ongoing; ply++) {
        GenerateMoves(*map, pos, list.get());
        if (list->count == 0) {
            why = "no moves while the game is on";
            break;
        }
        // end turns are rare so turns get long and units mix
        Move move = list->moves[Next(&rng) % static_cast<std::uint64_t>(list->count)];
        moves.push_back(move);
        ApplyMove(*map, &pos, move);
        (*plies)++;
        if (!Consistent(*map, pos, &why)) {
            break;
        }
    }
    auto record = std::make_unique<GameRecord>();
    record->map = *map;
    record->moves = moves;
    // every 50th game also goes through its text form and must replay to the same end
    if (why.empty() && seed % 50 == 0) {
        auto read = std::make_unique<GameRecord>();
        std::string error;
        if (!LoadGameText(WriteGameText(*record), read.get(), &error)) {
            why = "game text does not load back: " + error;
        } else {
            Position again;
            SetupStart(read->map, &again);
            for (const Move& move : read->moves) {
                ApplyMove(read->map, &again, move);
            }
            if (again.hash != pos.hash || read->moves.size() != moves.size()) {
                why = "game text replays to a different end";
            }
        }
    }
    if (why.empty()) {
        return "";
    }
    return why + "\n" + WriteGameText(*record);
}

} // namespace

// random games on random generated maps, many threads, checking the board after every move;
// failing games are written to fuzz-<seed>.sexp
int RunFuzz(int argc, char** argv) {
    std::uint64_t first = 1;
    std::uint64_t count = 1000;
    int threads = 4;
    for (int i = 0; i + 1 < argc; i += 2) {
        std::string arg = argv[i];
        if (arg == "--seed") {
            first = std::strtoull(argv[i + 1], nullptr, 10);
        } else if (arg == "--games") {
            count = std::strtoull(argv[i + 1], nullptr, 10);
        } else if (arg == "--threads") {
            threads = std::atoi(argv[i + 1]);
        }
    }
    std::atomic<std::uint64_t> next{0};
    std::atomic<std::uint64_t> plies{0};
    std::atomic<int> failures{0};
    std::mutex print;
    auto start = std::chrono::steady_clock::now();
    auto worker = [&]() {
        std::uint64_t local = 0;
        for (;;) {
            std::uint64_t index = next.fetch_add(1);
            if (index >= count) {
                break;
            }
            std::uint64_t seed = first + index;
            std::string failure = FuzzGame(seed, &local);
            if (failure.empty()) {
                continue;
            }
            failures++;
            std::string path = "fuzz-" + std::to_string(seed) + ".sexp";
            WriteWholeFile(path, failure.substr(failure.find('\n') + 1));
            std::lock_guard<std::mutex> guard(print);
            std::printf("seed %llu: %s -> %s\n", static_cast<unsigned long long>(seed),
                        failure.substr(0, failure.find('\n')).c_str(), path.c_str());
            std::fflush(stdout);
        }
        plies += local;
    };
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; t++) {
        pool.emplace_back(worker);
    }
    for (std::thread& thread : pool) {
        thread.join();
    }
    double seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::printf("%llu games, %llu moves, %d failures, %.1f s\n",
                static_cast<unsigned long long>(count),
                static_cast<unsigned long long>(plies.load()), failures.load(), seconds);
    return failures > 0 ? 1 : 0;
}

} // namespace shoal
