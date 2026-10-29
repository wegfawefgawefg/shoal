#include "game/position.hpp"
#include "io/map_file.hpp"
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

Move M(const Map& map, const char* text) {
    Move move;
    ParseMoveText(map, text, &move);
    return move;
}

bool Play(const Map& map, Position* pos, const char* text) {
    Move move = M(map, text);
    if (!IsLegal(map, *pos, move)) {
        std::printf("illegal: %s\n", text);
        return false;
    }
    ApplyMove(map, pos, move);
    return true;
}

const Unit* UnitOn(const Map& map, const Position& pos, const char* tile_name) {
    int tile = 0;
    ParseTileName(map, tile_name, &tile);
    if (pos.unit_at[tile] == kNoUnit) {
        return nullptr;
    }
    return &pos.units[pos.unit_at[tile]];
}

Unit MakeUnit(UnitType type, int owner, int hp, int ammo) {
    Unit unit;
    unit.type = type;
    unit.owner = static_cast<std::uint8_t>(owner);
    unit.hp = static_cast<std::uint8_t>(hp);
    unit.ammo = static_cast<std::uint8_t>(ammo);
    return unit;
}

void DamageTests(TestResults& r) {
    auto map = MapFrom(R"((map (tiles ".wc")))");
    Unit inf = MakeUnit(UnitType::Infantry, 0, 100, 0);
    Unit tank = MakeUnit(UnitType::Tank, 1, 100, 9);
    Unit art = MakeUnit(UnitType::Artillery, 0, 100, 9);
    Unit md = MakeUnit(UnitType::MdTank, 0, 100, 8);
    CHECK(r, AttackDamage(*map, inf, inf, 0) == 49);  // 55 * 0.9
    CHECK(r, AttackDamage(*map, tank, inf, 1) == 60); // machine gun into wood
    CHECK(r, AttackDamage(*map, art, tank, 0) == 63);
    CHECK(r, AttackDamage(*map, md, tank, 2) == 59); // 85 * 0.7 rounds down
    Unit hurt = inf;
    hurt.hp = 41; // shows 5
    CHECK(r, AttackDamage(*map, hurt, inf, 0) == 24); // 55 * 0.5 * 0.9 = 24.75
    Unit dry = tank;
    dry.ammo = 0; // out of shells: machine gun against a tank
    CHECK(r, AttackDamage(*map, dry, tank, 0) == 5);
}

void MoveGenTests(TestResults& r) {
    auto map = MapFrom(R"((map (tiles "..." "..." "...") (unit 0 inf a1) (unit 1 inf c3)))");
    Position pos;
    SetupStart(*map, &pos);
    auto list = std::make_unique<MoveList>();
    GenerateMoves(*map, pos, list.get());
    // 8 empty tiles to wait on, 2 of them next to the enemy, and end turn
    CHECK(r, list->count == 11);

    // treads cannot climb mountains; the enemy blocks the corridor
    auto walls = MapFrom(R"((map (tiles "..^" "^.." "...") (unit 0 tank a1) (unit 1 inf b3)))");
    SetupStart(*walls, &pos);
    CHECK(r, IsLegal(*walls, pos, M(*walls, "a1 c3")));
    CHECK(r, !IsLegal(*walls, pos, M(*walls, "a1 c1")));
    CHECK(r, !IsLegal(*walls, pos, M(*walls, "a1 a2")));
    CHECK(r, IsLegal(*walls, pos, M(*walls, "a1 b2 x b3")));
    CHECK(r, !IsLegal(*walls, pos, M(*walls, "a1 a3")));

    // artillery fires only without moving, at range 2..3
    auto range = MapFrom(R"((map (tiles "....." ".....") (unit 0 art a1) (unit 1 inf c1)
                             (unit 1 inf b2)))");
    SetupStart(*range, &pos);
    CHECK(r, IsLegal(*range, pos, M(*range, "a1 a1 x c1")));
    CHECK(r, IsLegal(*range, pos, M(*range, "a1 a1 x b2")));
    CHECK(r, !IsLegal(*range, pos, M(*range, "a1 a2 x c1")));
}

void CombatTests(TestResults& r) {
    auto map = MapFrom(R"((map (tiles "...") (unit 0 inf a1) (unit 1 inf c1)))");
    Position pos;
    SetupStart(*map, &pos);
    CHECK(r, Play(*map, &pos, "a1 b1 x c1"));
    const Unit* defender = UnitOn(*map, pos, "c1");
    const Unit* attacker = UnitOn(*map, pos, "b1");
    CHECK(r, defender != nullptr && defender->hp == 51);
    // the counter uses the defender's new hp: 55 * 0.6 * 0.9 = 29.7
    CHECK(r, attacker != nullptr && attacker->hp == 71);

    // a kill with no counter, and the last unit gone ends the game
    auto rout = MapFrom(R"((map (tiles "...") (unit 0 md a1) (unit 1 inf c1 10)))");
    SetupStart(*rout, &pos);
    CHECK(r, Play(*rout, &pos, "a1 b1 x c1"));
    CHECK(r, pos.result == Result::Win0);
    CHECK(r, pos.unit_count == 1);
}

void CaptureTests(TestResults& r) {
    auto map = MapFrom(R"((map (tiles "c.h" "...") (owner 1 c1) (unit 0 inf b1) (unit 0 inf a2)
                           (unit 1 inf c2 40)))");
    Position pos;
    SetupStart(*map, &pos);
    CHECK(r, Play(*map, &pos, "b1 a1 cap"));
    CHECK(r, pos.capture[map->property_at[0]] == 10);
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, Play(*map, &pos, "a1 a1 cap"));
    CHECK(r, pos.prop_owner[map->property_at[0]] == 0);
    CHECK(r, pos.capture[map->property_at[0]] == kCapturePoints);

    // moving off resets a capture in progress
    SetupStart(*map, &pos);
    CHECK(r, Play(*map, &pos, "b1 a1 cap"));
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, Play(*map, &pos, "a1 b1"));
    CHECK(r, pos.capture[map->property_at[0]] == kCapturePoints);

    // taking the hq wins
    auto hq = MapFrom(R"((map (tiles ".h") (owner 1 b1) (unit 0 inf a1) (unit 1 inf b1 30)))");
    SetupStart(*hq, &pos);
    CHECK(r, Play(*hq, &pos, "a1 a1 x b1"));
    CHECK(r, pos.result == Result::Win0);
}

void TurnTests(TestResults& r) {
    // repair on an owned city, paid from income; ammo refills
    auto map = MapFrom(R"((map (tiles "c..") (owner 0 a1) (unit 0 tank a1 50) (unit 1 inf c1)
                           (funds 0 0) (day-limit 2)))");
    Position pos;
    SetupStart(*map, &pos);
    // income 1000 arrives first, then the tank heals 2 hp for 1400... it cannot afford it,
    // so it heals 1 hp for 700
    const Unit* tank = UnitOn(*map, pos, "a1");
    CHECK(r, tank != nullptr && tank->hp == 60);
    CHECK(r, pos.funds[0] == 300);
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, pos.side == 1 && pos.day == 1);
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, pos.side == 0 && pos.day == 2);
    CHECK(r, Play(*map, &pos, "end"));
    // the day limit scores by properties: player 0 has the only city
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, pos.result == Result::Win0);
}

void JoinTests(TestResults& r) {
    auto map = MapFrom(R"((map (tiles "...." "....") (unit 0 inf a1) (unit 0 inf b1 40)
                           (unit 0 tank a2) (unit 1 inf d2)))");
    Position pos;
    SetupStart(*map, &pos);
    // only the hurt infantry can be joined; full units and other types cannot
    CHECK(r, IsLegal(*map, pos, M(*map, "a1 b1 join")));
    CHECK(r, !IsLegal(*map, pos, M(*map, "b1 a1 join")));
    CHECK(r, !IsLegal(*map, pos, M(*map, "a2 b1 join")));
    int funds = pos.funds[0];
    CHECK(r, Play(*map, &pos, "a1 b1 join"));
    const Unit* merged = UnitOn(*map, pos, "b1");
    CHECK(r, merged != nullptr && merged->hp == 100 && merged->done == 1);
    CHECK(r, UnitOn(*map, pos, "a1") == nullptr);
    CHECK(r, pos.unit_count == 3);
    // 10 + 4 board hp: the 4 past 10 come back at 100 each
    CHECK(r, pos.funds[0] == funds + 400);
}

void TransportTests(TestResults& r) {
    auto map = MapFrom(R"((map (tiles "....." "....." "~....") (unit 0 inf a1) (unit 0 apc b1)
                           (unit 0 tank c2) (unit 1 inf e3)))");
    Position pos;
    SetupStart(*map, &pos);
    CHECK(r, IsLegal(*map, pos, M(*map, "a1 b1 load")));
    CHECK(r, !IsLegal(*map, pos, M(*map, "c2 b1 load"))); // apcs carry foot soldiers only
    CHECK(r, Play(*map, &pos, "a1 b1 load"));
    const Unit* apc = UnitOn(*map, pos, "b1");
    CHECK(r, apc != nullptr && apc->cargo_count == 1 && apc->done == 0);
    CHECK(r, UnitOn(*map, pos, "a1") == nullptr);
    // the transport can still move and drop in the same turn; the dropped unit has acted
    CHECK(r, IsLegal(*map, pos, M(*map, "b1 d1 drop d2")));
    CHECK(r, Play(*map, &pos, "b1 d1 drop d2"));
    const Unit* dropped = UnitOn(*map, pos, "d2");
    CHECK(r, dropped != nullptr && dropped->type == UnitType::Infantry && dropped->done == 1);
    CHECK(r, UnitOn(*map, pos, "d1") != nullptr && UnitOn(*map, pos, "d1")->cargo_count == 0);

    // an apc refills the ammo of units next to it when its side's turn starts
    auto supply = MapFrom(R"((map (tiles "...") (unit 0 apc a1) (unit 0 tank b1)
                              (unit 1 inf c1)))");
    SetupStart(*supply, &pos);
    pos.units[pos.unit_at[1]].ammo = 0;
    CHECK(r, Play(*supply, &pos, "end"));
    CHECK(r, Play(*supply, &pos, "end"));
    CHECK(r, UnitOn(*supply, pos, "b1")->ammo == 9);

    // a destroyed transport takes its cargo with it, and the game sees it in the hash
    auto loaded = MapFrom(R"((map (tiles "...") (unit 0 inf a1) (unit 0 apc b1) (unit 1 md c1)))");
    SetupStart(*loaded, &pos);
    std::uint64_t before = pos.hash;
    CHECK(r, Play(*loaded, &pos, "a1 b1 load"));
    CHECK(r, pos.hash != before && pos.hash == ComputeHash(*loaded, pos));
    CHECK(r, Play(*loaded, &pos, "end"));
    CHECK(r, Play(*loaded, &pos, "c1 c1 x b1"));
    CHECK(r, pos.result == Result::Ongoing || pos.result == Result::Win1);
}

} // namespace

void RunRulesTests(TestResults& results) {
    JoinTests(results);
    TransportTests(results);
    DamageTests(results);
    MoveGenTests(results);
    CombatTests(results);
    CaptureTests(results);
    TurnTests(results);
}
