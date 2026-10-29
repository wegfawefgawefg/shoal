#include "io/map_file.hpp"
#include "io/map_gen.hpp"
#include "tools/commands.hpp"
#include "tools/engines.hpp"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace shoal {

namespace {

struct MatchOptions {
    EngineSpec a;
    EngineSpec b;
    std::vector<std::string> map_paths;
    int gen_size = 0;
    int gen_count = 0;
    bool gen_water = false;
    std::uint64_t gen_seed = 1;
    int threads = 4;
    std::string out_dir;
};

struct GameTask {
    int map_index = 0;
    bool a_first = true;
};

struct MatchTotals {
    std::mutex lock;
    int a_wins = 0;
    int b_wins = 0;
    int draws = 0;
    int first_wins = 0;
    int second_wins = 0;
};

// plays one game; returns its result and keeps the record
Result PlayGame(const Map& map, const EngineSpec& first, const EngineSpec& second,
                std::uint64_t seed, GameRecord* record) {
    auto engine0 = std::make_unique<Engine>();
    auto engine1 = std::make_unique<Engine>();
    EngineInit(engine0.get(), first, map, seed);
    EngineInit(engine1.get(), second, map, seed + 1);
    record->map = map;
    Position pos;
    SetupStart(map, &pos);
    for (int turn = 0; turn < 400 && pos.result == Result::Ongoing; turn++) {
        Engine* engine = pos.side == 0 ? engine0.get() : engine1.get();
        int score = 0;
        std::vector<Move> moves = PlayTurn(engine, map, pos, &score);
        for (const Move& move : moves) {
            if (pos.result != Result::Ongoing) {
                break;
            }
            ApplyMove(map, &pos, move);
            record->moves.push_back(move);
            record->evals.push_back(score);
        }
    }
    return pos.result;
}

double EloDiff(double score) {
    if (score <= 0.0 || score >= 1.0) {
        return score <= 0.0 ? -999.0 : 999.0;
    }
    return -400.0 * std::log10(1.0 / score - 1.0);
}

} // namespace

int RunMatch(int argc, char** argv) {
    MatchOptions options;
    bool have_a = false;
    bool have_b = false;
    for (int i = 0; i < argc; i++) {
        std::string arg = argv[i];
        bool has_value = i + 1 < argc;
        if (arg == "--a" && has_value) {
            have_a = ParseEngine(argv[++i], &options.a);
        } else if (arg == "--b" && has_value) {
            have_b = ParseEngine(argv[++i], &options.b);
        } else if (arg == "--gen" && i + 2 < argc) {
            options.gen_size = std::atoi(argv[++i]);
            options.gen_count = std::atoi(argv[++i]);
        } else if (arg == "--water") {
            options.gen_water = true;
        } else if (arg == "--seed" && has_value) {
            options.gen_seed = std::strtoull(argv[++i], nullptr, 10);
        } else if (arg == "--threads" && has_value) {
            options.threads = std::atoi(argv[++i]);
        } else if (arg == "--out" && has_value) {
            options.out_dir = argv[++i];
        } else if (arg.size() > 0 && arg[0] != '-') {
            options.map_paths.push_back(arg);
        } else {
            std::fprintf(stderr, "unknown option %s\n", arg.c_str());
            return 1;
        }
    }
    if (!have_a || !have_b) {
        std::fprintf(stderr, "usage: shoal match --a ENGINE --b ENGINE [maps...] "
                             "[--gen SIZE COUNT] [--water] [--seed S] [--threads T] [--out DIR]\n"
                             "  ENGINE: plan:t=0.5:turns=3:beam=6 | ply:t=0.5 | greedy | "
                             "random\n");
        return 1;
    }

    std::vector<std::unique_ptr<Map>> maps;
    for (const std::string& path : options.map_paths) {
        auto map = std::make_unique<Map>();
        std::string error;
        if (!LoadMapFile(path, map.get(), &error)) {
            std::fprintf(stderr, "%s: %s\n", path.c_str(), error.c_str());
            return 1;
        }
        maps.push_back(std::move(map));
    }
    for (int i = 0; i < options.gen_count; i++) {
        auto map = std::make_unique<Map>();
        MapGenOptions gen;
        gen.width = options.gen_size;
        gen.height = options.gen_size;
        gen.water = options.gen_water;
        gen.seed = options.gen_seed + static_cast<std::uint64_t>(i);
        GenerateMap(gen, map.get());
        maps.push_back(std::move(map));
    }

    std::vector<GameTask> tasks;
    for (int m = 0; m < static_cast<int>(maps.size()); m++) {
        tasks.push_back({m, true});
        tasks.push_back({m, false});
    }
    std::atomic<int> next{0};
    MatchTotals totals;
    auto worker = [&]() {
        for (;;) {
            int index = next.fetch_add(1);
            if (index >= static_cast<int>(tasks.size())) {
                return;
            }
            const GameTask& task = tasks[static_cast<std::size_t>(index)];
            const Map& map = *maps[static_cast<std::size_t>(task.map_index)];
            const EngineSpec& first = task.a_first ? options.a : options.b;
            const EngineSpec& second = task.a_first ? options.b : options.a;
            auto record = std::make_unique<GameRecord>();
            Result result =
                PlayGame(map, first, second, static_cast<std::uint64_t>(index), record.get());
            bool a_won = (result == Result::Win0 && task.a_first) ||
                         (result == Result::Win1 && !task.a_first);
            bool b_won = (result == Result::Win0 && !task.a_first) ||
                         (result == Result::Win1 && task.a_first);
            std::lock_guard<std::mutex> guard(totals.lock);
            totals.a_wins += a_won;
            totals.b_wins += b_won;
            totals.draws += result == Result::Draw;
            totals.first_wins += result == Result::Win0;
            totals.second_wins += result == Result::Win1;
            std::printf("%-16s a=p%d  %s  (%zu moves)\n", map.name.c_str(), task.a_first ? 0 : 1,
                        ResultTextFor(result), record->moves.size());
            std::fflush(stdout);
            if (!options.out_dir.empty()) {
                std::string path =
                    options.out_dir + "/" + map.name + (task.a_first ? "-a0" : "-a1") + ".sexp";
                WriteWholeFile(path, WriteGameText(*record));
            }
        }
    };
    std::vector<std::thread> threads;
    for (int t = 0; t < options.threads; t++) {
        threads.emplace_back(worker);
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
    int games = totals.a_wins + totals.b_wins + totals.draws;
    double score = games > 0 ? (totals.a_wins + 0.5 * totals.draws) / games : 0.5;
    std::printf("\n%s vs %s: %d games  +%d =%d -%d  score %.3f  elo %+.0f\n",
                options.a.text.c_str(), options.b.text.c_str(), games, totals.a_wins, totals.draws,
                totals.b_wins, score, EloDiff(score));
    std::printf("first player won %d, second won %d\n", totals.first_wins, totals.second_wins);
    return 0;
}

int RunGenMap(int argc, char** argv) {
    MapGenOptions gen;
    for (int i = 0; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--water") {
            gen.water = true;
            continue;
        }
        if (i + 1 >= argc) {
            break;
        }
        const char* value = argv[++i];
        if (arg == "--size") {
            gen.width = gen.height = std::atoi(value);
        } else if (arg == "--seed") {
            gen.seed = std::strtoull(value, nullptr, 10);
        } else if (arg == "--days") {
            gen.day_limit = std::atoi(value);
        } else if (arg == "--bases") {
            gen.bases_per_side = std::atoi(value);
        }
    }
    auto map = std::make_unique<Map>();
    GenerateMap(gen, map.get());
    std::printf("%s", WriteMapText(*map).c_str());
    return 0;
}

} // namespace shoal
