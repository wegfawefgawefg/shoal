#include "rules/units.hpp"

namespace shoal {

namespace {

// AWBW unit data (awbw.amarriner.com/units.php), AW2 tier
const UnitSpec kSpecs[kUnitTypeCount] = {
    {"Infantry", "inf", 1000, 3, MoveType::Foot, 0, 1, 1, true},
    {"Mech", "mech", 3000, 2, MoveType::Boot, 3, 1, 1, true},
    {"Recon", "recon", 4000, 8, MoveType::Tires, 0, 1, 1, false},
    {"Tank", "tank", 7000, 6, MoveType::Treads, 9, 1, 1, false},
    {"Md.Tank", "md", 16000, 5, MoveType::Treads, 8, 1, 1, false},
    {"Neotank", "neo", 22000, 6, MoveType::Treads, 9, 1, 1, false},
    {"Artillery", "art", 6000, 5, MoveType::Treads, 9, 2, 3, false},
    {"Rockets", "rkt", 15000, 5, MoveType::Tires, 6, 3, 5, false},
    {"Anti-Air", "aa", 8000, 6, MoveType::Treads, 9, 1, 1, false},
    {"Missiles", "msl", 12000, 4, MoveType::Tires, 6, 3, 5, false},
    {"APC", "apc", 5000, 6, MoveType::Treads, 0, 1, 1, false},
};

// rows attack, columns defend:
// inf mech recon tank md neo art rkt aa msl apc
// primary values are the AWBW damage chart (damage.php) where the primary weapon fires
const std::uint8_t kPrimary[kUnitTypeCount][kUnitTypeCount] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},                       // infantry
    {0, 0, 85, 55, 15, 15, 70, 85, 65, 85, 75},              // mech bazooka
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},                       // recon
    {0, 0, 85, 55, 15, 15, 70, 85, 65, 85, 75},              // tank cannon
    {0, 0, 105, 85, 55, 45, 105, 105, 105, 105, 105},        // md tank cannon
    {0, 0, 125, 105, 75, 55, 115, 125, 115, 125, 125},       // neotank cannon
    {90, 85, 80, 70, 45, 40, 75, 80, 75, 80, 70},            // artillery
    {95, 90, 90, 80, 55, 50, 80, 85, 85, 90, 80},            // rockets
    {105, 105, 60, 25, 10, 5, 50, 55, 45, 55, 50},           // anti-air vulcan
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},                       // missiles hit air only
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},                       // apc
};

// machine guns; the infantry, mech-vs-foot and recon rows match the AWBW chart, the
// out-of-ammo rows against vehicles are the AW2 values from memory (docs/rules.md)
const std::uint8_t kSecondary[kUnitTypeCount][kUnitTypeCount] = {
    {55, 45, 12, 5, 1, 1, 15, 25, 5, 25, 14},                // infantry
    {65, 55, 18, 6, 1, 1, 32, 35, 6, 35, 20},                // mech
    {70, 65, 35, 6, 1, 1, 45, 55, 4, 28, 45},                // recon
    {75, 70, 40, 6, 1, 1, 45, 55, 5, 30, 45},                // tank
    {105, 95, 45, 8, 1, 1, 45, 55, 7, 35, 45},               // md tank
    {125, 115, 65, 10, 1, 1, 65, 75, 17, 55, 65},            // neotank
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},                       // artillery
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},                       // rockets
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},                       // anti-air
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},                       // missiles
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},                       // apc
};

} // namespace

const UnitSpec& Spec(UnitType type) {
    return kSpecs[static_cast<int>(type)];
}

bool IsIndirect(UnitType type) {
    return Spec(type).range_max > 1;
}

int PrimaryDamage(UnitType attacker, UnitType defender) {
    return kPrimary[static_cast<int>(attacker)][static_cast<int>(defender)];
}

int SecondaryDamage(UnitType attacker, UnitType defender) {
    return kSecondary[static_cast<int>(attacker)][static_cast<int>(defender)];
}

bool UnitTypeFromShortName(std::string_view name, UnitType* out) {
    for (int i = 0; i < kUnitTypeCount; i++) {
        if (kSpecs[i].short_name == name) {
            *out = static_cast<UnitType>(i);
            return true;
        }
    }
    return false;
}

} // namespace shoal
