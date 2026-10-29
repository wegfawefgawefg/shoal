#pragma once

#include "rules/units.hpp"

#include <cstdint>

namespace shoal {

// rows attack, columns defend:
//   inf mech recon tank md neo art rkt aa msl apc | fighter bomber bcopter tcopter |
//   bship cruiser lander sub
// every number is the AWBW damage chart (damage.php, read 2026-10-29), put under the weapon
// that fires it; the chart lists one value per pair
inline constexpr std::uint8_t kPrimaryDamage[kUnitTypeCount][kUnitTypeCount] = {
    // infantry: machine gun only
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // mech bazooka
    {0, 0, 85, 55, 15, 15, 70, 85, 65, 85, 75, 0, 0, 0, 0, 0, 0, 0, 0},
    // recon: machine gun only
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // tank cannon
    {0, 0, 85, 55, 15, 15, 70, 85, 65, 85, 75, 0, 0, 0, 0, 1, 5, 10, 1},
    // md tank cannon
    {0, 0, 105, 85, 55, 45, 105, 105, 105, 105, 105, 0, 0, 0, 0, 10, 45, 35, 10},
    // neotank cannon
    {0, 0, 125, 105, 75, 55, 115, 125, 115, 125, 125, 0, 0, 0, 0, 15, 50, 40, 15},
    // artillery
    {90, 85, 80, 70, 45, 40, 75, 80, 75, 80, 70, 0, 0, 0, 0, 40, 65, 55, 60},
    // rockets
    {95, 90, 90, 80, 55, 50, 80, 85, 85, 90, 80, 0, 0, 0, 0, 55, 85, 60, 85},
    // anti-air vulcan
    {105, 105, 60, 25, 10, 5, 50, 55, 45, 55, 50, 65, 75, 120, 120, 0, 0, 0, 0},
    // missiles: air only
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 100, 100, 120, 120, 0, 0, 0, 0},
    // apc
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // fighter missiles: air only
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 55, 100, 100, 100, 0, 0, 0, 0},
    // bomber bombs: ground and sea
    {110, 110, 105, 105, 95, 90, 105, 105, 95, 105, 105, 0, 0, 0, 0, 75, 85, 95, 95},
    // b-copter missiles: vehicles and ships
    {0, 0, 55, 55, 25, 20, 65, 65, 25, 65, 60, 0, 0, 0, 0, 25, 55, 25, 25},
    // t-copter
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // battleship cannon
    {95, 90, 90, 80, 55, 50, 80, 85, 85, 90, 80, 0, 0, 0, 0, 50, 95, 95, 95},
    // cruiser anti-sub missiles
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 90},
    // lander
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // sub torpedoes
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 55, 25, 95, 55},
};

// machine guns, which never run out. The AWBW chart's numbers where the gun is the weapon that
// fires (against foot soldiers, copters, the cruiser's guns against air); what a mech, tank,
// md tank, neotank or b-copter does to vehicles once its primary ammo is gone comes from the
// AW2 chart at warswiki.org/wiki/Damage_chart (MG columns), read 2026-10-29
inline constexpr std::uint8_t kSecondaryDamage[kUnitTypeCount][kUnitTypeCount] = {
    // infantry
    {55, 45, 12, 5, 1, 1, 15, 25, 5, 25, 14, 0, 0, 7, 30, 0, 0, 0, 0},
    // mech
    {65, 55, 18, 6, 1, 1, 32, 35, 6, 35, 20, 0, 0, 9, 35, 0, 0, 0, 0},
    // recon
    {70, 65, 35, 6, 1, 1, 45, 55, 4, 28, 45, 0, 0, 10, 35, 0, 0, 0, 0},
    // tank
    {75, 70, 40, 6, 1, 1, 45, 55, 5, 30, 45, 0, 0, 10, 40, 0, 0, 0, 0},
    // md tank
    {105, 95, 45, 8, 1, 1, 45, 55, 7, 35, 45, 0, 0, 12, 45, 0, 0, 0, 0},
    // neotank
    {125, 115, 65, 10, 1, 1, 65, 75, 17, 55, 65, 0, 0, 22, 55, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // artillery
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // rockets
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // anti-air
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // missiles
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // apc
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // fighter
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // bomber
    // b-copter
    {75, 75, 30, 6, 1, 1, 25, 35, 6, 35, 20, 0, 0, 65, 95, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // t-copter
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // battleship
    // cruiser anti-air guns
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 55, 65, 115, 115, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // lander
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // sub
};

// base damage percent; 0 means the weapon cannot hit that target
inline int PrimaryDamage(UnitType attacker, UnitType defender) {
    return kPrimaryDamage[static_cast<int>(attacker)][static_cast<int>(defender)];
}

inline int SecondaryDamage(UnitType attacker, UnitType defender) {
    return kSecondaryDamage[static_cast<int>(attacker)][static_cast<int>(defender)];
}

// a unit with any weapon at all
inline bool IsArmed(UnitType type) {
    for (int t = 0; t < kUnitTypeCount; t++) {
        if (kPrimaryDamage[static_cast<int>(type)][t] > 0 ||
            kSecondaryDamage[static_cast<int>(type)][t] > 0) {
            return true;
        }
    }
    return false;
}

} // namespace shoal
