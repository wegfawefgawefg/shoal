#include "game/check.hpp"

namespace shoal {

bool Consistent(const Map& map, const Position& pos, std::string* why) {
    int tile_count = map.width * map.height;
    int seen = 0;
    for (int tile = 0; tile < tile_count; tile++) {
        std::uint8_t index = pos.unit_at[tile];
        if (index == kNoUnit) {
            continue;
        }
        seen++;
        if (index >= pos.unit_count || pos.units[index].tile != tile) {
            *why = "unit_at points at the wrong unit on " + TileName(map, tile);
            return false;
        }
    }
    if (seen != pos.unit_count) {
        *why = "unit_at and the unit list disagree";
        return false;
    }
    for (int i = 0; i < pos.unit_count; i++) {
        const Unit& unit = pos.units[i];
        if (unit.hp < 1 || unit.hp > 100 || unit.ammo > Spec(unit.type).ammo) {
            *why = "unit out of range on " + TileName(map, unit.tile);
            return false;
        }
        bool cargo_ok = unit.cargo_count <= CargoCapacity(unit.type);
        for (int c = 0; c < unit.cargo_count && cargo_ok; c++) {
            const Cargo& cargo = unit.cargo[c];
            cargo_ok = CarriesType(unit.type, cargo.type) && cargo.fuel <= Spec(cargo.type).fuel;
        }
        bool done_ok = unit.done <= 1 || (unit.done == kDropping && unit.cargo_count > 0);
        if (unit.fuel > Spec(unit.type).fuel || (unit.dived && unit.type != UnitType::Sub) ||
            !cargo_ok || !done_ok) {
            *why = "bad fuel, dive, cargo or done on " + TileName(map, unit.tile);
            return false;
        }
        if (MoveCost(map.tiles[unit.tile], Spec(unit.type).move_type) == 0) {
            *why = "unit on impassable terrain at " + TileName(map, unit.tile);
            return false;
        }
    }
    if (pos.hash != ComputeHash(map, pos)) {
        *why = "stale hash";
        return false;
    }
    if (pos.funds[0] < 0 || pos.funds[1] < 0) {
        *why = "negative funds";
        return false;
    }
    return true;
}

} // namespace shoal
