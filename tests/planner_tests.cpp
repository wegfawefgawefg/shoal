#include "io/map_file.hpp"
#include "search/eval.hpp"
#include "search/planner.hpp"
#include "search/search.hpp"
#include "tests.hpp"

#include <memory>
#include <string>

using namespace shoal;

namespace {

std::unique_ptr<Map> MapFrom(const char* text) {
    auto map = std::make_unique<Map>();
    std::string error;
    if (!LoadMapText(text, map.get(), &error)) {
        std::printf("map error: %s\n", error.c_str());
    }
    return map;
}

PlannerResult Plan(const Map& map, const Position& pos) {
    auto planner = std::make_unique<Planner>();
    PlannerInit(planner.get(), map);
    PlannerLimits limits;
    limits.seconds = 0.2;
    limits.quiet = true;
    return PlanTurn(planner.get(), pos, limits);
}

bool TurnHas(const Map& map, const TurnPlan& turn, const char* text) {
    for (const Move& move : turn.moves) {
        if (MoveText(map, move) == text) {
            return true;
        }
    }
    return false;
}

} // namespace

void RunPlannerTests(TestResults& r) {
    // an infantry on the enemy hq with the capture nearly done finishes it
    auto hq = MapFrom(R"((map (tiles "...." "...h") (owner 1 d2) (unit 0 inf d2 100)
                          (unit 0 inf a1) (unit 1 tank a2)))");
    Position pos;
    SetupStart(*hq, &pos);
    pos.capture[hq->property_at[7]] = 8;
    pos.hash = ComputeHash(*hq, pos);
    PlannerResult win = Plan(*hq, pos);
    CHECK(r, TurnHas(*hq, win.turn, "d2 d2 cap"));
    CHECK(r, win.score > kWinThreshold);

    // a tank in reach of the last enemy unit, at 1 hp, kills it and wins by rout
    auto kill = MapFrom(R"((map (tiles "....." "....." ".....") (unit 0 tank a2)
                            (unit 1 inf c2 10)))");
    SetupStart(*kill, &pos);
    PlannerResult shot = Plan(*kill, pos);
    bool attacked = false;
    for (const Move& move : shot.turn.moves) {
        attacked = attacked || (move.kind == MoveKind::Attack &&
                                MoveText(*kill, move).find("x c2") != std::string::npos);
    }
    CHECK(r, attacked);
    CHECK(r, shot.score > kWinThreshold);

    // the ply search finds the same hq win
    auto searcher = std::make_unique<Searcher>();
    SearcherInit(searcher.get(), *hq, 16);
    SetupStart(*hq, &pos);
    pos.capture[hq->property_at[7]] = 8;
    pos.hash = ComputeHash(*hq, pos);
    SearchLimits limits;
    limits.max_depth = 3;
    limits.quiet = true;
    SearchResult found = Search(searcher.get(), pos, limits);
    CHECK(r, MoveText(*hq, found.best) == "d2 d2 cap");
}
