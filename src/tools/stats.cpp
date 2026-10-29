#include "io/map_file.hpp"
#include "tools/commands.hpp"

#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace shoal {

namespace {

constexpr int kPhases = 3; // days 1-5, 6-10, 11+

struct TypeStats {
    int built = 0;
    int built_by_phase[kPhases] = {};
    std::int64_t dealt = 0; // funds of enemy hp removed by this type's attacks and counters
    std::int64_t taken = 0; // funds of this type's hp lost
    int lost = 0;
    int crashed = 0; // ran out of fuel
};

struct Totals {
    int games = 0;
    int results[4] = {};
    std::int64_t days = 0;
    TypeStats types[kUnitTypeCount];
    std::int64_t winner_builds[kUnitTypeCount] = {};
};

int Phase(int day) {
    return day <= 5 ? 0 : (day <= 10 ? 1 : 2);
}

// hp lost per unit identified by tile, before and after a move
void CountCombat(const Map& map, const Position& before, const Position& after, const Move& move,
                 Totals* totals) {
    if (move.kind != MoveKind::Attack) {
        return;
    }
    const Unit& attacker = before.units[before.unit_at[move.from]];
    const Unit& defender = before.units[before.unit_at[move.target]];
    int defender_hp =
        after.unit_at[move.target] == kNoUnit ? 0 : after.units[after.unit_at[move.target]].hp;
    int attacker_hp =
        after.unit_at[move.to] == kNoUnit ? 0 : after.units[after.unit_at[move.to]].hp;
    int defender_loss = Spec(defender.type).cost * (defender.hp - defender_hp) / 100;
    int attacker_loss = Spec(attacker.type).cost * (attacker.hp - attacker_hp) / 100;
    TypeStats& a = totals->types[static_cast<int>(attacker.type)];
    TypeStats& d = totals->types[static_cast<int>(defender.type)];
    a.dealt += defender_loss;
    d.taken += defender_loss;
    d.dealt += attacker_loss;
    a.taken += attacker_loss;
    d.lost += defender_hp == 0;
    a.lost += attacker_hp == 0;
    (void)map;
}

// air and sea units missing after a turn start went down for lack of fuel
void CountCrashes(const Position& before, const Position& after, Totals* totals) {
    int side = before.side ^ 1;
    for (int i = 0; i < before.unit_count; i++) {
        const Unit& unit = before.units[i];
        if (unit.owner != side || Spec(unit.type).fuel_per_day == 0) {
            continue;
        }
        std::uint8_t index = after.unit_at[unit.tile];
        if (index == kNoUnit || after.units[index].type != unit.type) {
            totals->types[static_cast<int>(unit.type)].crashed++;
        }
    }
}

void CountGame(const GameRecord& game, Totals* totals) {
    Position pos;
    SetupStart(game.map, &pos);
    int builds[2][kUnitTypeCount] = {};
    for (const Move& move : game.moves) {
        Position before = pos;
        ApplyMove(game.map, &pos, move);
        if (move.kind == MoveKind::Build) {
            TypeStats& t = totals->types[static_cast<int>(move.build)];
            t.built++;
            t.built_by_phase[Phase(before.day)]++;
            builds[before.side][static_cast<int>(move.build)]++;
        }
        CountCombat(game.map, before, pos, move, totals);
        if (move.kind == MoveKind::EndTurn && before.side != pos.side) {
            CountCrashes(before, pos, totals);
        }
    }
    totals->games++;
    totals->results[static_cast<int>(pos.result)]++;
    totals->days += pos.day;
    int winner = pos.result == Result::Win0 ? 0 : (pos.result == Result::Win1 ? 1 : -1);
    if (winner >= 0) {
        for (int type = 0; type < kUnitTypeCount; type++) {
            totals->winner_builds[type] += builds[winner][type];
        }
    }
}

} // namespace

int RunStats(int argc, char** argv) {
    std::vector<std::string> paths;
    for (int i = 0; i < argc; i++) {
        std::filesystem::path path = argv[i];
        if (std::filesystem::is_directory(path)) {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                if (entry.path().extension() == ".sexp") {
                    paths.push_back(entry.path().string());
                }
            }
        } else {
            paths.push_back(path.string());
        }
    }
    auto totals = std::make_unique<Totals>();
    for (const std::string& path : paths) {
        auto game = std::make_unique<GameRecord>();
        std::string error;
        if (!LoadGameFile(path, game.get(), &error)) {
            std::fprintf(stderr, "%s: %s\n", path.c_str(), error.c_str());
            continue;
        }
        CountGame(*game, totals.get());
    }
    if (totals->games == 0) {
        std::fprintf(stderr, "no games\n");
        return 1;
    }
    std::printf("%d games: first player won %d, second won %d, draws %d; %.1f days on average\n\n",
                totals->games, totals->results[1], totals->results[2], totals->results[3],
                static_cast<double>(totals->days) / totals->games);
    std::printf("%-10s %6s %7s %7s %7s %10s %10s %6s %7s %9s %7s\n", "unit", "built", "d1-5",
                "d6-10", "d11+", "dealt", "taken", "lost", "ratio", "by-winner", "crashed");
    for (int type = 0; type < kUnitTypeCount; type++) {
        const TypeStats& t = totals->types[type];
        if (t.built == 0 && t.dealt == 0 && t.taken == 0) {
            continue;
        }
        double ratio =
            t.taken > 0 ? static_cast<double>(t.dealt) / static_cast<double>(t.taken) : 0.0;
        std::printf("%-10s %6d %7d %7d %7d %10lld %10lld %6d %7.2f %9lld %7d\n",
                    std::string(Spec(static_cast<UnitType>(type)).name).c_str(), t.built,
                    t.built_by_phase[0], t.built_by_phase[1], t.built_by_phase[2],
                    static_cast<long long>(t.dealt), static_cast<long long>(t.taken), t.lost, ratio,
                    static_cast<long long>(totals->winner_builds[type]), t.crashed);
    }
    std::printf(
        "\ndealt/taken are funds of hit points removed in attacks this unit took part "
        "in;\nratio = dealt / taken; by-winner = units the winning side built; crashed = ran out "
        "of fuel\n");
    return 0;
}

} // namespace shoal
