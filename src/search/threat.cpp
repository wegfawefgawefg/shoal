#include "search/threat.hpp"

namespace shoal {

namespace {

bool CanStrike(const Unit& attacker, const Unit& defender) {
    if (PrimaryDamage(attacker.type, defender.type) > 0 && attacker.ammo > 0) {
        return true;
    }
    return SecondaryDamage(attacker.type, defender.type) > 0;
}

void MarkRing(const Map& map, int center, int range_min, int range_max, std::uint64_t bit,
              ThreatMap* threats) {
    int x0 = TileX(map, center);
    int y0 = TileY(map, center);
    for (int dy = -range_max; dy <= range_max; dy++) {
        for (int dx = -range_max; dx <= range_max; dx++) {
            int dist = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
            int x = x0 + dx;
            int y = y0 + dy;
            if (dist < range_min || dist > range_max || x < 0 || y < 0 || x >= map.width ||
                y >= map.height) {
                continue;
            }
            threats->attackers[TileAt(map, x, y)] |= bit;
        }
    }
}

} // namespace

void BuildThreats(const Map& map, const Position& pos, int attacker_side, ThreatMap* threats) {
    int tile_count = map.width * map.height;
    threats->attacker_side = attacker_side;
    threats->slot_count = 0;
    for (int tile = 0; tile < tile_count; tile++) {
        threats->attackers[tile] = 0;
    }
    int reach[kMaxTiles];
    for (int i = 0; i < pos.unit_count; i++) {
        const Unit& unit = pos.units[i];
        if (unit.owner != attacker_side) {
            continue;
        }
        int slot = threats->slot_count++;
        threats->slot_tile[slot] = unit.tile;
        std::uint64_t bit = 1ull << slot;
        const UnitSpec& spec = Spec(unit.type);
        if (spec.ammo == 0 && SecondaryDamage(unit.type, UnitType::Infantry) == 0) {
            continue; // unarmed
        }
        if (spec.range_max > 1) {
            MarkRing(map, unit.tile, spec.range_min, spec.range_max, bit, threats);
            continue;
        }
        // the attacker's own side moves next, so its units are not done; reach ignores that
        int count = ReachableTiles(map, pos, unit, reach);
        for (int r = 0; r < count; r++) {
            MarkRing(map, reach[r], 1, 1, bit, threats);
        }
    }
}

int Exposure(const Map& map, const Position& pos, const ThreatMap& threats, const Unit& unit,
             int tile, int second_pct) {
    std::uint64_t bits = threats.attackers[tile];
    int best = 0;
    int second = 0;
    Unit placed = unit;
    placed.tile = static_cast<std::uint16_t>(tile);
    while (bits != 0) {
        int slot = __builtin_ctzll(bits);
        bits &= bits - 1;
        std::uint8_t index = pos.unit_at[threats.slot_tile[slot]];
        if (index == kNoUnit) {
            continue;
        }
        const Unit& attacker = pos.units[index];
        if (attacker.owner != threats.attacker_side || !CanStrike(attacker, placed)) {
            continue;
        }
        int damage = AttackDamage(map, attacker, placed, tile);
        if (damage > best) {
            second = best;
            best = damage;
        } else if (damage > second) {
            second = damage;
        }
    }
    int loss = best + second * second_pct / 100;
    if (loss > unit.hp) {
        loss = unit.hp;
    }
    return Spec(unit.type).cost * loss / 100;
}

} // namespace shoal
