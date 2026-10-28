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
    Count,
};

constexpr int kUnitTypeCount = static_cast<int>(UnitType::Count);

enum class MoveType : std::uint8_t {
    Foot,
    Boot,
    Treads,
    Tires,
    Count,
};

struct UnitSpec {
    std::string_view name;      // long name for docs and the viewer
    std::string_view short_name; // atom used in map and game files
    int cost = 0;
    int move = 0;
    MoveType move_type = MoveType::Foot;
    int ammo = 0; // primary weapon ammo; 0 means no primary weapon
    int range_min = 1;
    int range_max = 1;
    bool can_capture = false;
};

const UnitSpec& Spec(UnitType type);
bool IsIndirect(UnitType type);

// base damage percent; 0 means the weapon cannot hit that target
int PrimaryDamage(UnitType attacker, UnitType defender);
int SecondaryDamage(UnitType attacker, UnitType defender);

bool UnitTypeFromShortName(std::string_view name, UnitType* out);

} // namespace shoal
