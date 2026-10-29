#include "rule_helpers.hpp"
#include "tests.hpp"

using namespace shoal;

namespace {

Unit* UnitAt(const Map& map, Position* pos, const char* tile_name) {
    int tile = 0;
    ParseTileName(map, tile_name, &tile);
    if (pos->unit_at[tile] == kNoUnit) {
        return nullptr;
    }
    return &pos->units[pos->unit_at[tile]];
}

bool Legal(const Map& map, const Position& pos, const char* text) {
    return IsLegal(map, pos, M(map, text));
}

// spot checks against the AWBW chart; air units get no terrain stars
void AirSeaDamageTests(TestResults& r) {
    auto map = MapFrom(R"((map (tiles ".^sp")))");
    Unit fighter = MakeUnit(UnitType::Fighter, 0, 100, 9);
    Unit bomber = MakeUnit(UnitType::Bomber, 1, 100, 9);
    Unit bcopter = MakeUnit(UnitType::BCopter, 0, 100, 6);
    Unit tcopter = MakeUnit(UnitType::TCopter, 1, 100, 0);
    Unit tank = MakeUnit(UnitType::Tank, 1, 100, 9);
    Unit md = MakeUnit(UnitType::MdTank, 1, 100, 8);
    Unit inf = MakeUnit(UnitType::Infantry, 1, 100, 0);
    Unit aa = MakeUnit(UnitType::AntiAir, 1, 100, 9);
    Unit msl = MakeUnit(UnitType::Missiles, 1, 100, 6);
    Unit bship = MakeUnit(UnitType::Battleship, 0, 100, 9);
    Unit cruiser = MakeUnit(UnitType::Cruiser, 0, 100, 9);
    Unit sub = MakeUnit(UnitType::Sub, 1, 100, 6);
    Unit lander = MakeUnit(UnitType::Lander, 1, 100, 0);
    CHECK(r, AttackDamage(*map, fighter, bomber, 1) == 100); // over a mountain, still no cover
    CHECK(r, AttackDamage(*map, fighter, bcopter, 0) == 100);
    CHECK(r, AttackDamage(*map, bcopter, tank, 0) == 49); // 55 * 0.9
    CHECK(r, AttackDamage(*map, bcopter, inf, 0) == 67);  // machine gun 75 * 0.9
    CHECK(r, AttackDamage(*map, bcopter, tcopter, 1) == 95);
    CHECK(r, AttackDamage(*map, bomber, md, 0) == 85); // 95 * 0.9
    CHECK(r, AttackDamage(*map, bomber, cruiser, 2) == 85);
    CHECK(r, AttackDamage(*map, aa, bcopter, 0) == 120);
    CHECK(r, AttackDamage(*map, aa, fighter, 0) == 65);
    CHECK(r, AttackDamage(*map, msl, bomber, 0) == 100);
    CHECK(r, AttackDamage(*map, tank, bcopter, 0) == 10); // machine gun
    CHECK(r, AttackDamage(*map, inf, tcopter, 0) == 30);
    CHECK(r, AttackDamage(*map, bship, tank, 0) == 72); // 80 * 0.9
    CHECK(r, AttackDamage(*map, bship, sub, 2) == 95);
    CHECK(r, AttackDamage(*map, cruiser, sub, 2) == 90);
    CHECK(r, AttackDamage(*map, cruiser, bomber, 2) == 65); // anti-air guns
    CHECK(r, AttackDamage(*map, sub, lander, 2) == 95);
    CHECK(r, AttackDamage(*map, sub, MakeUnit(UnitType::Battleship, 1, 100, 9), 3) == 38);
    CHECK(r, AttackDamage(*map, tank, sub, 2) == 1);
    CHECK(r, !CanHit(fighter, tank));
    CHECK(r, !CanHit(bomber, fighter));
    CHECK(r, !CanHit(cruiser, MakeUnit(UnitType::Battleship, 1, 100, 9)));
    CHECK(r, !CanHit(inf, fighter));
    Unit dry = bcopter;
    dry.ammo = 0; // missiles gone: the gun against a tank
    CHECK(r, AttackDamage(*map, dry, tank, 0) == 5);
}

void AirSeaMoveTests(TestResults& r) {
    // copters fly over mountains and sea at one point a tile
    auto air = MapFrom(R"((map (tiles "^^^^^^^^" "ssssssss") (unit 0 bcopter a1)
                           (unit 1 inf h1)))");
    Position pos;
    SetupStart(*air, &pos);
    CHECK(r, Legal(*air, pos, "a1 g1"));
    CHECK(r, Legal(*air, pos, "a1 f2"));
    CHECK(r, !Legal(*air, pos, "a1 g2")); // 7 tiles
    CHECK(r, Play(*air, &pos, "a1 f2"));
    CHECK(r, UnitOn(*air, pos, "f2")->fuel == 93);

    // a tank of fuel caps the move
    SetupStart(*air, &pos);
    UnitAt(*air, &pos, "a1")->fuel = 3;
    CHECK(r, Legal(*air, pos, "a1 d1"));
    CHECK(r, !Legal(*air, pos, "a1 e1"));
    UnitAt(*air, &pos, "a1")->fuel = 0;
    CHECK(r, !Legal(*air, pos, "a1 b1"));

    // ships keep to sea, reef costs them 2, landers also take shoals
    auto sea = MapFrom(R"((map (tiles "ssr_.." "sssss.") (unit 0 lander a1) (unit 0 cruiser a2)
                      (unit 0 tank e1) (unit 1 inf f2)))");
    SetupStart(*sea, &pos);
    CHECK(r, Legal(*sea, pos, "a2 e2"));
    CHECK(r, !Legal(*sea, pos, "a2 d1")); // shoal
    CHECK(r, Legal(*sea, pos, "a1 d1"));  // 1 + 2 + 1
    CHECK(r, !Legal(*sea, pos, "a1 e1"));
    CHECK(r, !Legal(*sea, pos, "e1 e2")); // tanks do not swim
    CHECK(r, Legal(*sea, pos, "e1 d1"));  // but take the shoal
    CHECK(r, Play(*sea, &pos, "a2 c1"));  // 1 + 1 + 2
    CHECK(r, UnitOn(*sea, pos, "c1")->fuel == 95);
}

void FuelTests(TestResults& r) {
    auto map = MapFrom(R"((map (tiles "...." "....") (unit 0 fighter a1) (unit 0 bcopter b1)
                           (unit 0 tank c1) (unit 1 inf d2) (day-limit 9)))");
    Position pos;
    SetupStart(*map, &pos);
    // nothing burns on day 1; from day 2 each air unit burns its daily fuel
    CHECK(r, UnitOn(*map, pos, "a1")->fuel == 99);
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, UnitOn(*map, pos, "a1")->fuel == 94);
    CHECK(r, UnitOn(*map, pos, "b1")->fuel == 97);
    CHECK(r, UnitOn(*map, pos, "c1")->fuel == 70);
    // a plane left empty goes down at the start of its turn
    UnitAt(*map, &pos, "a1")->fuel = 5;
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, UnitOn(*map, pos, "a1") != nullptr); // the other side's turn burns nothing of ours
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, UnitOn(*map, pos, "a1") == nullptr);
    CHECK(r, pos.result == Result::Ongoing);
    CHECK(r, pos.hash == ComputeHash(*map, pos));

    // losing the last unit to fuel loses the game
    auto alone = MapFrom(R"((map (tiles "...") (unit 0 fighter a1) (unit 1 inf c1)))");
    SetupStart(*alone, &pos);
    UnitAt(*alone, &pos, "a1")->fuel = 4;
    CHECK(r, Play(*alone, &pos, "end"));
    CHECK(r, Play(*alone, &pos, "end"));
    CHECK(r, pos.result == Result::Win1);

    // a dived sub burns 5 a day, a surfaced one 1
    auto subs = MapFrom(R"((map (tiles "ss" "..") (unit 0 sub a1) (unit 0 sub b1)
                            (unit 1 inf b2)))");
    SetupStart(*subs, &pos);
    CHECK(r, Play(*subs, &pos, "a1 a1 dive"));
    CHECK(r, Play(*subs, &pos, "end"));
    CHECK(r, Play(*subs, &pos, "end"));
    CHECK(r, UnitOn(*subs, pos, "a1")->fuel == 55);
    CHECK(r, UnitOn(*subs, pos, "b1")->fuel == 59);

    // an apc next to a plane fills its tank; two planes joining add fuel up to full
    auto apc = MapFrom(R"((map (tiles "...." "....") (unit 0 apc a1) (unit 0 bcopter b1)
                           (unit 0 bcopter d1 50) (unit 1 inf d2)))");
    SetupStart(*apc, &pos);
    UnitAt(*apc, &pos, "b1")->fuel = 10;
    UnitAt(*apc, &pos, "d1")->fuel = 20;
    CHECK(r, Play(*apc, &pos, "end"));
    CHECK(r, Play(*apc, &pos, "end"));
    CHECK(r, UnitOn(*apc, pos, "b1")->fuel == 99);
    CHECK(r, UnitOn(*apc, pos, "d1")->fuel == 18);
    UnitAt(*apc, &pos, "b1")->fuel = 90;
    CHECK(r, Play(*apc, &pos, "b1 d1 join"));
    CHECK(r, UnitOn(*apc, pos, "d1")->fuel == 99);
}

void SupplyTests(TestResults& r) {
    // own airports repair and refuel air units, ports ships; cities do neither
    auto map = MapFrom(R"((map (tiles "a.cp" "...s") (owner 0 a1) (owner 0 c1) (owner 0 d1)
                           (funds 50000 0) (unit 0 fighter a1 50) (unit 0 bcopter c1 50)
                           (unit 0 bship d1 50) (unit 0 tank b2 50) (unit 1 inf a2)))");
    Position pos;
    SetupStart(*map, &pos);
    CHECK(r, UnitOn(*map, pos, "a1")->hp == 70);
    CHECK(r, UnitOn(*map, pos, "c1")->hp == 50);
    CHECK(r, UnitOn(*map, pos, "d1")->hp == 70);
    CHECK(r, pos.funds[0] == 50000 + 3000 - 4000 - 5600);
    UnitAt(*map, &pos, "a1")->fuel = 20;
    UnitAt(*map, &pos, "a1")->ammo = 0;
    UnitAt(*map, &pos, "c1")->fuel = 20;
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, Play(*map, &pos, "end"));
    const Unit* fighter = UnitOn(*map, pos, "a1");
    CHECK(r, fighter->fuel == 99 && fighter->ammo == 9 && fighter->hp == 90);
    CHECK(r, UnitOn(*map, pos, "c1")->fuel == 18);
    // the daily burn comes first: an empty plane on its airport still goes down
    UnitAt(*map, &pos, "a1")->fuel = 5;
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, UnitOn(*map, pos, "a1") == nullptr);

    // airports build air units, ports ships, bases ground units, each from the build list
    auto build = MapFrom(R"((map (tiles "abp" "s.s") (owner 0 a1) (owner 0 b1) (owner 0 c1)
                             (funds 100000 0) (build inf fighter bship) (unit 1 inf b2)))");
    SetupStart(*build, &pos);
    CHECK(r, Legal(*build, pos, "a1 build fighter"));
    CHECK(r, Legal(*build, pos, "c1 build bship"));
    CHECK(r, Legal(*build, pos, "b1 build inf"));
    CHECK(r, !Legal(*build, pos, "a1 build inf"));
    CHECK(r, !Legal(*build, pos, "b1 build fighter"));
    CHECK(r, !Legal(*build, pos, "c1 build fighter"));
    CHECK(r, !Legal(*build, pos, "a1 build bomber")); // not on the list
    CHECK(r, Play(*build, &pos, "a1 build fighter"));
    CHECK(r, UnitOn(*build, pos, "a1")->fuel == 99);
}

void AirSeaTransportTests(TestResults& r) {
    // a t-copter carries one foot soldier over anything and drops it on land
    auto heli = MapFrom(R"((map (tiles "..^ss." "......") (unit 0 inf a1) (unit 0 tcopter b1)
                            (unit 0 tank a2) (unit 1 inf f2)))");
    Position pos;
    SetupStart(*heli, &pos);
    CHECK(r, !Legal(*heli, pos, "a2 b1 load"));
    CHECK(r, Play(*heli, &pos, "a1 b1 load"));
    CHECK(r, !Legal(*heli, pos, "b1 c1 drop d1")); // into the sea
    CHECK(r, Legal(*heli, pos, "b1 e1 drop f1"));
    CHECK(r, Play(*heli, &pos, "b1 e1 drop f1"));
    const Unit* dropped = UnitOn(*heli, pos, "f1");
    CHECK(r, dropped != nullptr && dropped->fuel == 98 && dropped->done == 1);
    CHECK(r, UnitOn(*heli, pos, "e1")->fuel == 96 && UnitOn(*heli, pos, "e1")->done == 1);

    // a lander takes two ground units and lands them only from a shoal or a port; it may drop
    // the second where it stands after the first, and nothing else
    auto boat = MapFrom(R"((map (tiles "..__ss" "..ssss") (unit 0 mech a1) (unit 0 tank b1)
                            (unit 0 lander c1) (unit 1 inf a2)))");
    SetupStart(*boat, &pos);
    CHECK(r, Play(*boat, &pos, "b1 c1 load"));
    CHECK(r, Play(*boat, &pos, "a1 c1 load"));
    CHECK(r, UnitOn(*boat, pos, "c1")->cargo_count == 2);
    CHECK(r, !Legal(*boat, pos, "c1 e1 drop d1")); // at sea
    CHECK(r, Legal(*boat, pos, "c1 d1 drop c1"));
    CHECK(r, Legal(*boat, pos, "c1 c1 drop d1 2"));
    CHECK(r, Play(*boat, &pos, "c1 c1 drop b1"));
    const Unit* lander = UnitOn(*boat, pos, "c1");
    CHECK(r, lander->done == kDropping && lander->cargo_count == 1);
    CHECK(r, UnitOn(*boat, pos, "b1")->type == UnitType::Tank);
    CHECK(r, !Legal(*boat, pos, "c1 d1 drop c1"));   // no moving after a drop
    CHECK(r, !Legal(*boat, pos, "c1 c1 drop d1 2")); // one cargo left, in slot 1
    CHECK(r, Play(*boat, &pos, "c1 c1 drop d1"));
    CHECK(r, UnitOn(*boat, pos, "d1")->type == UnitType::Mech);
    CHECK(r, UnitOn(*boat, pos, "c1")->done == 1 && UnitOn(*boat, pos, "c1")->cargo_count == 0);
    CHECK(r, pos.hash == ComputeHash(*boat, pos));

    // a cruiser carries two copters and nothing else
    auto ship = MapFrom(R"((map (tiles "sss" "...") (unit 0 cruiser a1) (unit 0 bcopter b1)
                            (unit 0 tcopter c1) (unit 0 inf a2) (unit 1 inf c2)))");
    SetupStart(*ship, &pos);
    CHECK(r, Play(*ship, &pos, "b1 a1 load"));
    CHECK(r, Play(*ship, &pos, "c1 a1 load"));
    CHECK(r, UnitOn(*ship, pos, "a1")->cargo_count == 2);
    CHECK(r, Legal(*ship, pos, "a1 a1 drop b1 2"));
    CHECK(r, !Legal(*ship, pos, "a2 a1 load"));
}

void SubTests(TestResults& r) {
    auto map = MapFrom(R"((map (tiles "ss." "sss") (unit 0 cruiser a1) (unit 1 sub b1)
                           (unit 0 tank c1) (unit 0 bomber b2) (day-limit 9)))");
    Position pos;
    SetupStart(*map, &pos);
    CHECK(r, Legal(*map, pos, "c1 c1 x b1"));
    CHECK(r, Legal(*map, pos, "b2 b2 x b1"));
    CHECK(r, Legal(*map, pos, "a1 a1 x b1"));
    UnitAt(*map, &pos, "b1")->dived = 1;
    // only cruisers and subs find a dived sub
    CHECK(r, !Legal(*map, pos, "c1 c1 x b1"));
    CHECK(r, !Legal(*map, pos, "b2 b2 x b1"));
    CHECK(r, Legal(*map, pos, "a1 a1 x b1"));
    UnitAt(*map, &pos, "b1")->dived = 0;
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, Legal(*map, pos, "b1 b1 dive"));
    CHECK(r, !Legal(*map, pos, "b1 b1 surface"));
    CHECK(r, Play(*map, &pos, "b1 b1 dive"));
    CHECK(r, UnitOn(*map, pos, "b1")->dived == 1 && UnitOn(*map, pos, "b1")->done == 1);
    CHECK(r, MoveText(*map, M(*map, "b1 b1 dive")) == "b1 b1 dive");
    CHECK(r, MoveText(*map, M(*map, "a1 b1 surface")) == "a1 b1 surface");
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, !Legal(*map, pos, "c1 c1 x b1"));
    CHECK(r, Play(*map, &pos, "end"));
    CHECK(r, Legal(*map, pos, "b1 b1 surface"));
    CHECK(r, !Legal(*map, pos, "b1 b1 dive"));
    CHECK(r, Legal(*map, pos, "b1 b1 x a1")); // a dived sub still fires
}

void BattleshipTests(TestResults& r) {
    auto map = MapFrom(R"((map (tiles "s......." "ss......") (unit 0 bship a1) (unit 1 tank g1)
                           (unit 1 tank h2) (unit 1 inf b1) (unit 1 inf f1)))");
    Position pos;
    SetupStart(*map, &pos);
    CHECK(r, Legal(*map, pos, "a1 a1 x g1")); // 6 away
    CHECK(r, Legal(*map, pos, "a1 a1 x f1"));
    CHECK(r, !Legal(*map, pos, "a1 a1 x h2")); // 8 away
    CHECK(r, !Legal(*map, pos, "a1 a1 x b1")); // too close
    CHECK(r, !Legal(*map, pos, "a1 a2 x f1")); // never after moving
    CHECK(r, Play(*map, &pos, "a1 a1 x g1"));
    CHECK(r, UnitOn(*map, pos, "a1")->hp == 100); // indirect: no counter
    CHECK(r, UnitOn(*map, pos, "g1")->hp == 100 - 72);
}

} // namespace

void RunAirSeaTests(TestResults& results) {
    AirSeaDamageTests(results);
    AirSeaMoveTests(results);
    FuelTests(results);
    SupplyTests(results);
    AirSeaTransportTests(results);
    SubTests(results);
    BattleshipTests(results);
}
