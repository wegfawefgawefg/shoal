#pragma once

#include <cstdint>
#include <string_view>

namespace shoal {

// unit types; the order is the column order of the damage tables
enum class UnitType : std::uint8_t {
    Infantry,
    Mech,
    Recon,
    Tank,
    MdTank,
    Neotank,
    Artillery,
    Rockets,
    AntiAir,
    Missiles,
    Apc,
    Fighter,
    Bomber,
    BCopter,
    TCopter,
    Battleship,
    Cruiser,
    Lander,
    Sub,
    Count,
};

constexpr int kUnitTypeCount = static_cast<int>(UnitType::Count);

// the movement columns of the AWBW terrain chart
enum class MoveType : std::uint8_t {
    Foot,
    Boot,
    Treads,
    Tires,
    Air,
    Sea,
    Lander,
    Count,
};

enum class Domain : std::uint8_t {
    Ground,
    Air,
    Sea,
};

struct UnitSpec {
    std::string_view name;       // long name for docs and the viewer
    std::string_view short_name; // atom used in map and game files
    int cost = 0;
    int move = 0;
    MoveType move_type = MoveType::Foot;
    int ammo = 0; // primary weapon ammo; 0 means no primary weapon
    int range_min = 1;
    int range_max = 1;
    bool can_capture = false;
    int fuel = 99;        // full tank
    int fuel_per_day = 0; // burnt at the start of the owner's turn
};

// a dived sub burns this much a day instead
constexpr int kSubDivedFuelPerDay = 5;

// AWBW unit data (awbw.amarriner.com/units.php), AW2 tier
inline constexpr UnitSpec kUnitSpecs[kUnitTypeCount] = {
    {"Infantry", "inf", 1000, 3, MoveType::Foot, 0, 1, 1, true, 99, 0},
    {"Mech", "mech", 3000, 2, MoveType::Boot, 3, 1, 1, true, 70, 0},
    {"Recon", "recon", 4000, 8, MoveType::Tires, 0, 1, 1, false, 80, 0},
    {"Tank", "tank", 7000, 6, MoveType::Treads, 9, 1, 1, false, 70, 0},
    {"Md.Tank", "md", 16000, 5, MoveType::Treads, 8, 1, 1, false, 50, 0},
    {"Neotank", "neo", 22000, 6, MoveType::Treads, 9, 1, 1, false, 99, 0},
    {"Artillery", "art", 6000, 5, MoveType::Treads, 9, 2, 3, false, 50, 0},
    {"Rockets", "rkt", 15000, 5, MoveType::Tires, 6, 3, 5, false, 50, 0},
    {"Anti-Air", "aa", 8000, 6, MoveType::Treads, 9, 1, 1, false, 60, 0},
    {"Missiles", "msl", 12000, 4, MoveType::Tires, 6, 3, 5, false, 50, 0},
    {"APC", "apc", 5000, 6, MoveType::Treads, 0, 1, 1, false, 70, 0},
    {"Fighter", "fighter", 20000, 9, MoveType::Air, 9, 1, 1, false, 99, 5},
    {"Bomber", "bomber", 22000, 7, MoveType::Air, 9, 1, 1, false, 99, 5},
    {"B-Copter", "bcopter", 9000, 6, MoveType::Air, 6, 1, 1, false, 99, 2},
    {"T-Copter", "tcopter", 5000, 6, MoveType::Air, 0, 1, 1, false, 99, 2},
    {"Battleship", "bship", 28000, 5, MoveType::Sea, 9, 2, 6, false, 99, 1},
    {"Cruiser", "cruiser", 18000, 6, MoveType::Sea, 9, 1, 1, false, 99, 1},
    {"Lander", "lander", 12000, 6, MoveType::Lander, 0, 1, 1, false, 99, 1},
    {"Sub", "sub", 20000, 5, MoveType::Sea, 6, 1, 1, false, 60, 1},
};

inline const UnitSpec& Spec(UnitType type) {
    return kUnitSpecs[static_cast<int>(type)];
}

inline Domain DomainOf(UnitType type) {
    MoveType move_type = Spec(type).move_type;
    if (move_type == MoveType::Air) {
        return Domain::Air;
    }
    if (move_type == MoveType::Sea || move_type == MoveType::Lander) {
        return Domain::Sea;
    }
    return Domain::Ground;
}

inline bool IsIndirect(UnitType type) {
    return Spec(type).range_max > 1;
}

// only these find a dived sub
inline bool HitsDived(UnitType type) {
    return type == UnitType::Cruiser || type == UnitType::Sub;
}

// how many units a transport carries, and whether it carries this type; a unit carrying
// cargo never boards another transport
inline int CargoCapacity(UnitType transport) {
    switch (transport) {
    case UnitType::Apc:
    case UnitType::TCopter:
        return 1;
    case UnitType::Lander:
    case UnitType::Cruiser:
        return 2;
    default:
        return 0;
    }
}

inline bool CarriesType(UnitType transport, UnitType passenger) {
    switch (transport) {
    case UnitType::Apc:
    case UnitType::TCopter:
        return passenger == UnitType::Infantry || passenger == UnitType::Mech;
    case UnitType::Lander:
        return DomainOf(passenger) == Domain::Ground;
    case UnitType::Cruiser:
        return passenger == UnitType::BCopter || passenger == UnitType::TCopter;
    default:
        return false;
    }
}

bool UnitTypeFromShortName(std::string_view name, UnitType* out);

} // namespace shoal
