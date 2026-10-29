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

// one tunable: the field, its step, and the range it may take
struct TuneParam {
    const char* name;
    double step;
    int low;
    int high;
};

const TuneParam kParams[] = {
    {"exposure", 6, 0, 300},
    {"second_hit", 12, 0, 150},
    {"capture_reach", 250, 0, 8000},
    {"capture_per_turn", 70, 0, 3000},
    {"fight_turns", 1, 1, 10},
    {"fight_share", 8, 0, 300},
    {"base_block", 120, 0, 3000},
    {"idle_funds", 5, 0, 80},
    {"excess_funds", 8, 0, 100},
    {"property_base", 300, 0, 10000},
    {"property_days", 1, 1, 20},
    {"capture_share", 8, 0, 100},
    {"value_inf", 6, 40, 250},
    {"value_mech", 6, 40, 250},
    {"value_recon", 6, 40, 250},
    {"value_tank", 6, 40, 250},
    {"value_md", 6, 40, 250},
    {"value_art", 6, 40, 250},
    {"value_aa", 6, 40, 250},
    {"value_neo", 6, 40, 250},
    {"value_rkt", 6, 40, 250},
    {"value_apc", 6, 40, 250},
};
constexpr int kParamCount = sizeof(kParams) / sizeof(kParams[0]);

std::uint64_t NextRandom(std::uint64_t* state) {
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

PlanWeights ToWeights(const double* theta) {
    PlanWeights weights;
    for (int i = 0; i < kParamCount; i++) {
        int value = static_cast<int>(std::lround(theta[i]));
        if (value < kParams[i].low) {
            value = kParams[i].low;
        }
        if (value > kParams[i].high) {
            value = kParams[i].high;
        }
        SetPlanWeight(&weights, kParams[i].name, value);
    }
    return weights;
}

Result PlayOne(const Map& map, const EngineSpec& first, const EngineSpec& second,
               std::uint64_t seed) {
    auto engine0 = std::make_unique<Engine>();
    auto engine1 = std::make_unique<Engine>();
    EngineInit(engine0.get(), first, map, seed);
    EngineInit(engine1.get(), second, map, seed + 1);
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
        }
    }
    return pos.result;
}

} // namespace

// SPSA over the planner weights: each round plays weights+delta against weights-delta on
// fresh maps, both colours, and steps toward the winner
int RunTune(int argc, char** argv) {
    int rounds = 1000;
    int maps_per_round = 8;
    int threads = 8;
    double seconds = 0.1;
    double rate = 1.0;
    std::uint64_t seed = 1;
    for (int i = 0; i + 1 < argc; i += 2) {
        std::string arg = argv[i];
        if (arg == "--rounds") {
            rounds = std::atoi(argv[i + 1]);
        } else if (arg == "--maps") {
            maps_per_round = std::atoi(argv[i + 1]);
        } else if (arg == "--threads") {
            threads = std::atoi(argv[i + 1]);
        } else if (arg == "--time") {
            seconds = std::atof(argv[i + 1]);
        } else if (arg == "--rate") {
            rate = std::atof(argv[i + 1]);
        } else if (arg == "--seed") {
            seed = std::strtoull(argv[i + 1], nullptr, 10);
        }
    }
    double theta[kParamCount];
    PlanWeights start;
    for (int i = 0; i < kParamCount; i++) {
        theta[i] = *PlanWeightField(&start, kParams[i].name);
    }
    std::uint64_t rng = seed * 0x9e3779b97f4a7c15ull + 7;
    for (int round = 0; round < rounds; round++) {
        double delta[kParamCount];
        double plus[kParamCount];
        double minus[kParamCount];
        for (int i = 0; i < kParamCount; i++) {
            delta[i] = (NextRandom(&rng) & 1) ? 1.0 : -1.0;
            plus[i] = theta[i] + kParams[i].step * delta[i];
            minus[i] = theta[i] - kParams[i].step * delta[i];
        }
        EngineSpec a;
        EngineSpec b;
        ParseEngine("plan", &a);
        ParseEngine("plan", &b);
        a.seconds = b.seconds = seconds;
        a.weights = ToWeights(plus);
        b.weights = ToWeights(minus);

        std::vector<std::unique_ptr<Map>> maps;
        for (int m = 0; m < maps_per_round; m++) {
            auto map = std::make_unique<Map>();
            MapGenOptions gen;
            // small to big maps, with one to three bases a side so money is sometimes plentiful
            gen.width = gen.height = 9 + static_cast<int>(NextRandom(&rng) % 10);
            gen.bases_per_side = 1 + static_cast<int>(NextRandom(&rng) % 3);
            gen.day_limit = 15 + gen.width / 2;
            gen.seed = NextRandom(&rng);
            GenerateMap(gen, map.get());
            maps.push_back(std::move(map));
        }
        std::atomic<int> next{0};
        std::atomic<int> a_points{0}; // two per win, one per draw
        int games = maps_per_round * 2;
        auto worker = [&]() {
            for (;;) {
                int index = next.fetch_add(1);
                if (index >= games) {
                    return;
                }
                const Map& map = *maps[static_cast<std::size_t>(index / 2)];
                bool a_first = (index & 1) == 0;
                Result result = a_first ? PlayOne(map, a, b, static_cast<std::uint64_t>(index))
                                        : PlayOne(map, b, a, static_cast<std::uint64_t>(index));
                bool a_won = (result == Result::Win0) == a_first && result != Result::Draw;
                a_points += result == Result::Draw ? 1 : (a_won ? 2 : 0);
            }
        };
        std::vector<std::thread> pool;
        for (int t = 0; t < threads; t++) {
            pool.emplace_back(worker);
        }
        for (std::thread& thread : pool) {
            thread.join();
        }
        // +1 when the plus side took everything, -1 when the minus side did
        double outcome = (a_points.load() - games) / static_cast<double>(games);
        for (int i = 0; i < kParamCount; i++) {
            theta[i] += rate * kParams[i].step * outcome * delta[i];
            if (theta[i] < kParams[i].low) {
                theta[i] = kParams[i].low;
            }
            if (theta[i] > kParams[i].high) {
                theta[i] = kParams[i].high;
            }
        }
        std::printf("round %d outcome %+.2f  %s\n", round, outcome,
                    PlanWeightsText(ToWeights(theta)).c_str());
        std::fflush(stdout);
    }
    return 0;
}

} // namespace shoal
