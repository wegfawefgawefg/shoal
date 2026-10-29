#include "io/map_file.hpp"
#include "tools/commands.hpp"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace shoal {

namespace {

// where a unit stands for a duel: a terrain, and for cities whether it is its own (it heals)
struct Footing {
    const char* name;
    Terrain terrain;
    bool own = false;
};

const Footing kFootings[] = {
    {"road", Terrain::Road},          {"plain", Terrain::Plain},
    {"wood", Terrain::Wood},          {"city", Terrain::City},
    {"own city", Terrain::City, true}, {"mountain", Terrain::Mountain},
};

struct DuelResult {
    bool possible = false; // both can stand there and the striker can hit
    int winner = -1;       // 0 the unit that stepped in, 1 the striker, -1 nobody within the limit
    int hp_left = 0;       // the winner's board hp
    int first_stepper = 0; // board hp of both after the first exchange
    int first_striker = 0;
};

// the stepper stands on `step`, the striker next to it on `strike` and hits first; then they
// trade blows where they stand until one is destroyed (40 turns at most)
DuelResult RunDuel(UnitType stepper, UnitType striker, const Footing& step,
                   const Footing& strike) {
    DuelResult result;
    if (MoveCost(step.terrain, Spec(stepper).move_type) == 0 ||
        MoveCost(strike.terrain, Spec(striker).move_type) == 0) {
        return result;
    }
    auto map = std::make_unique<Map>();
    map->width = 2;
    map->height = 1;
    map->tiles[0] = strike.terrain;
    map->tiles[1] = step.terrain;
    map->day_limit = 60;
    map->start_funds[0] = map->start_funds[1] = 1000000;
    IndexProperties(map.get());
    for (int prop = 0; prop < map->property_count; prop++) {
        map->start_owner[prop] = kNeutral;
    }
    // the striker is player 0 so it moves first; an own city belongs to its holder
    if (strike.own) {
        map->start_owner[map->property_at[0]] = 0;
    }
    if (step.own) {
        map->start_owner[map->property_at[1]] = 1;
    }
    map->start_units.push_back({striker, 0, 0, 100, -1});
    map->start_units.push_back({stepper, 1, 1, 100, -1});
    Position pos;
    SetupStart(*map, &pos);

    Move strike_move;
    ParseMoveText(*map, "a1 a1 x b1", &strike_move);
    Move answer_move;
    ParseMoveText(*map, "b1 b1 x a1", &answer_move);
    if (!IsLegal(*map, pos, strike_move)) {
        return result;
    }
    result.possible = true;
    for (int turn = 0; turn < 80 && pos.result == Result::Ongoing; turn++) {
        const Move& attack = pos.side == 0 ? strike_move : answer_move;
        if (IsLegal(*map, pos, attack)) {
            ApplyMove(*map, &pos, attack);
        }
        if (turn == 0) {
            std::uint8_t a = pos.unit_at[0];
            std::uint8_t b = pos.unit_at[1];
            result.first_striker = a == kNoUnit ? 0 : DisplayHp(pos.units[a].hp);
            result.first_stepper = b == kNoUnit ? 0 : DisplayHp(pos.units[b].hp);
        }
        if (pos.result == Result::Ongoing) {
            ApplyMove(*map, &pos, Move{});
        }
    }
    if (pos.result == Result::Win0) {
        result.winner = 1;
        result.hp_left = DisplayHp(pos.units[pos.unit_at[0]].hp);
    } else if (pos.result == Result::Win1) {
        result.winner = 0;
        result.hp_left = DisplayHp(pos.units[pos.unit_at[1]].hp);
    }
    return result;
}

std::string Cell(const DuelResult& r) {
    if (!r.possible) {
        return "-";
    }
    std::string first = std::to_string(r.first_stepper) + "/" + std::to_string(r.first_striker);
    if (r.winner == 0) {
        return "WIN " + std::to_string(r.hp_left) + " (" + first + ")";
    }
    if (r.winner == 1) {
        return "loss (" + first + ")";
    }
    return "hold (" + first + ")";
}

void PrintTable(UnitType stepper, UnitType striker) {
    std::printf("\n%s steps in, %s strikes first\n", std::string(Spec(stepper).name).c_str(),
                std::string(Spec(striker).name).c_str());
    std::printf("%-12s", "stepper on");
    for (const Footing& strike : kFootings) {
        std::printf(" | %-16s", strike.name);
    }
    std::printf("\n");
    for (const Footing& step : kFootings) {
        std::printf("%-12s", step.name);
        for (const Footing& strike : kFootings) {
            std::printf(" | %-16s", Cell(RunDuel(stepper, striker, step, strike)).c_str());
        }
        std::printf("\n");
    }
}

const UnitType kMatrixUnits[] = {UnitType::Infantry, UnitType::Mech,    UnitType::Recon,
                                 UnitType::Tank,     UnitType::MdTank,  UnitType::Neotank,
                                 UnitType::AntiAir,  UnitType::BCopter};

// the weakest footing (in kFootings order) the stepper can stand on and still win when the
// striker hits first from `strike`; "never" when even its own city loses
std::string LeastFooting(UnitType stepper, UnitType striker, const Footing& strike) {
    bool any = false;
    for (const Footing& step : kFootings) {
        DuelResult r = RunDuel(stepper, striker, step, strike);
        any = any || r.possible;
        if (r.possible && r.winner == 0) {
            return step.name;
        }
    }
    return any ? "never" : "-";
}

void PrintMatrix(const Footing& strike) {
    std::printf("\nleast footing to step in and win, striker on %s (rows step in, columns strike)\n",
                strike.name);
    std::printf("%-10s", "");
    for (UnitType striker : kMatrixUnits) {
        std::printf(" | %-9s", std::string(Spec(striker).short_name).c_str());
    }
    std::printf("\n");
    for (UnitType stepper : kMatrixUnits) {
        std::printf("%-10s", std::string(Spec(stepper).short_name).c_str());
        for (UnitType striker : kMatrixUnits) {
            std::printf(" | %-9s", LeastFooting(stepper, striker, strike).c_str());
        }
        std::printf("\n");
    }
}

} // namespace

// shoal duel [stepper striker]: who wins an even fight when one side steps into the other's
// range on a given footing and the other strikes first from its own footing. Cells: the
// stepper's result, its board hp left when it wins, and (stepper/striker) hp after the first
// exchange. "hold" means neither can finish the other.
int RunDuelTables(int argc, char** argv) {
    std::vector<std::pair<UnitType, UnitType>> pairs;
    if (argc >= 1 && std::string(argv[0]) == "--matrix") {
        PrintMatrix(kFootings[1]); // plain
        PrintMatrix(kFootings[2]); // wood
        PrintMatrix(kFootings[3]); // city
        return 0;
    }
    if (argc >= 2) {
        UnitType a;
        UnitType b;
        if (!UnitTypeFromShortName(argv[0], &a) || !UnitTypeFromShortName(argv[1], &b)) {
            std::fprintf(stderr, "usage: shoal duel [stepper striker]\n");
            return 1;
        }
        pairs.push_back({a, b});
    } else {
        for (UnitType type : {UnitType::Infantry, UnitType::Mech, UnitType::Recon,
                              UnitType::Tank, UnitType::MdTank, UnitType::Neotank,
                              UnitType::AntiAir}) {
            pairs.push_back({type, type});
        }
    }
    for (const auto& [stepper, striker] : pairs) {
        PrintTable(stepper, striker);
    }
    return 0;
}

} // namespace shoal
