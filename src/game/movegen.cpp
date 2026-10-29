#include "game/position.hpp"

#include <cstring>

namespace shoal {

namespace {

constexpr int kMaxMovePoints = 10;

} // namespace

namespace {

// reach with the friendly-occupied tiles a unit could join reported separately
// friendly-occupied tiles the unit could end on are reported apart: units it could join, and
// transports it could load into
struct SpecialDests {
    int joins[kMaxUnits];
    int join_count = 0;
    int loads[kMaxUnits];
    int load_count = 0;
};

int Reach(const Map& map, const Position& pos, const Unit& unit, int out[kMaxTiles],
          SpecialDests* special) {
    const UnitSpec& spec = Spec(unit.type);
    int tile_count = map.width * map.height;
    std::int8_t left[kMaxTiles];
    std::memset(left, -1, static_cast<std::size_t>(tile_count));

    // settle tiles from most move points left to least; every step costs at least one point,
    // so a tile is final when its bucket comes up; a tile enters each bucket at most once
    static thread_local std::int16_t buckets[kMaxMovePoints + 1][kMaxTiles];
    int bucket_size[kMaxMovePoints + 1] = {};
    left[unit.tile] = static_cast<std::int8_t>(spec.move);
    buckets[spec.move][bucket_size[spec.move]++] = static_cast<std::int16_t>(unit.tile);
    for (int points = spec.move; points > 0; points--) {
        for (int i = 0; i < bucket_size[points]; i++) {
            int tile = buckets[points][i];
            if (left[tile] != points) {
                continue;
            }
            int around[4];
            int around_count = Neighbours(map, tile, around);
            for (int n = 0; n < around_count; n++) {
                int to = around[n];
                int cost = MoveCost(map.tiles[to], spec.move_type);
                if (cost == 0 || cost > points) {
                    continue;
                }
                std::uint8_t occupant = pos.unit_at[to];
                if (occupant != kNoUnit && pos.units[occupant].owner != unit.owner) {
                    continue;
                }
                int remain = points - cost;
                if (left[to] >= remain) {
                    continue;
                }
                left[to] = static_cast<std::int8_t>(remain);
                buckets[remain][bucket_size[remain]++] = static_cast<std::int16_t>(to);
            }
        }
    }

    int count = 0;
    for (int tile = 0; tile < tile_count; tile++) {
        if (left[tile] < 0) {
            continue;
        }
        std::uint8_t occupant = pos.unit_at[tile];
        if (occupant != kNoUnit && tile != unit.tile) {
            if (special != nullptr && CanJoin(unit, pos.units[occupant])) {
                special->joins[special->join_count++] = tile;
            } else if (special != nullptr && CanLoad(unit, pos.units[occupant])) {
                special->loads[special->load_count++] = tile;
            }
            continue;
        }
        out[count++] = tile;
    }
    return count;
}

} // namespace

bool CanJoin(const Unit& unit, const Unit& target) {
    return target.owner == unit.owner && target.type == unit.type && target.tile != unit.tile &&
           DisplayHp(target.hp) < 10 && unit.cargo_count == 0 && target.cargo_count == 0;
}

bool CanLoad(const Unit& unit, const Unit& transport) {
    return transport.owner == unit.owner && transport.tile != unit.tile &&
           CarriesType(transport.type, unit.type) &&
           transport.cargo_count < CargoCapacity(transport.type);
}

int ReachableTiles(const Map& map, const Position& pos, const Unit& unit, int out[kMaxTiles]) {
    return Reach(map, pos, unit, out, nullptr);
}

namespace {

bool CanHit(const Unit& attacker, const Unit& defender) {
    if (PrimaryDamage(attacker.type, defender.type) > 0 && attacker.ammo > 0) {
        return true;
    }
    return SecondaryDamage(attacker.type, defender.type) > 0;
}

void Push(MoveList* list, MoveKind kind, int from, int to, int target) {
    if (list->count >= kMaxMoves) {
        return;
    }
    Move& move = list->moves[list->count++];
    move.kind = kind;
    move.build = UnitType::Infantry;
    move.from = static_cast<std::uint16_t>(from);
    move.to = static_cast<std::uint16_t>(to);
    move.target = static_cast<std::uint16_t>(target);
    move.slot = 0;
}

// a transport that ends on `dest` drops each cargo on a free neighbour it can stand on; the
// tile the transport left is free by then
void PushUnloads(const Map& map, const Position& pos, const Unit& transport, int dest,
                 MoveList* list) {
    int around[4];
    int around_count = Neighbours(map, dest, around);
    for (int slot = 0; slot < transport.cargo_count; slot++) {
        MoveType move_type = Spec(transport.cargo[slot].type).move_type;
        for (int n = 0; n < around_count; n++) {
            int drop = around[n];
            bool free = pos.unit_at[drop] == kNoUnit || drop == transport.tile;
            if (!free || MoveCost(map.tiles[drop], move_type) == 0) {
                continue;
            }
            Push(list, MoveKind::Unload, transport.tile, dest, drop);
            list->moves[list->count - 1].slot = static_cast<std::uint8_t>(slot);
        }
    }
}

void PushAttacksFrom(const Map& map, const Position& pos, const Unit& unit, int from, int dest,
                     MoveList* list) {
    const UnitSpec& spec = Spec(unit.type);
    if (spec.range_max > 1) {
        // indirect fire: never after moving, any tile in the range ring
        if (dest != from) {
            return;
        }
        int x0 = TileX(map, from);
        int y0 = TileY(map, from);
        for (int dy = -spec.range_max; dy <= spec.range_max; dy++) {
            for (int dx = -spec.range_max; dx <= spec.range_max; dx++) {
                int dist = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
                if (dist < spec.range_min || dist > spec.range_max) {
                    continue;
                }
                int x = x0 + dx;
                int y = y0 + dy;
                if (x < 0 || y < 0 || x >= map.width || y >= map.height) {
                    continue;
                }
                int target = TileAt(map, x, y);
                std::uint8_t occupant = pos.unit_at[target];
                if (occupant == kNoUnit || pos.units[occupant].owner == unit.owner) {
                    continue;
                }
                if (CanHit(unit, pos.units[occupant])) {
                    Push(list, MoveKind::Attack, from, dest, target);
                }
            }
        }
        return;
    }
    int around[4];
    int around_count = Neighbours(map, dest, around);
    for (int n = 0; n < around_count; n++) {
        int target = around[n];
        std::uint8_t occupant = pos.unit_at[target];
        if (occupant == kNoUnit || pos.units[occupant].owner == unit.owner) {
            continue;
        }
        if (CanHit(unit, pos.units[occupant])) {
            Push(list, MoveKind::Attack, from, dest, target);
        }
    }
}

void PushBuilds(const Map& map, const Position& pos, MoveList* list) {
    for (int prop = 0; prop < map.property_count; prop++) {
        int tile = map.property_tiles[prop];
        if (map.tiles[tile] != Terrain::Base || pos.prop_owner[prop] != pos.side) {
            continue;
        }
        if (pos.unit_at[tile] != kNoUnit || pos.unit_count >= kMaxUnits) {
            continue;
        }
        for (int type = 0; type < kUnitTypeCount; type++) {
            if (!map.buildable[type]) {
                continue;
            }
            if (Spec(static_cast<UnitType>(type)).cost > pos.funds[pos.side]) {
                continue;
            }
            if (list->count >= kMaxMoves) {
                return;
            }
            Move& move = list->moves[list->count++];
            move.kind = MoveKind::Build;
            move.build = static_cast<UnitType>(type);
            move.from = static_cast<std::uint16_t>(tile);
            move.to = static_cast<std::uint16_t>(tile);
            move.target = static_cast<std::uint16_t>(tile);
            move.slot = 0;
        }
    }
}

} // namespace

void GenerateMoves(const Map& map, const Position& pos, MoveList* list) {
    list->count = 0;
    if (pos.result != Result::Ongoing) {
        return;
    }
    int dests[kMaxTiles];
    SpecialDests special;
    for (int i = 0; i < pos.unit_count; i++) {
        const Unit& unit = pos.units[i];
        if (unit.owner != pos.side || unit.done) {
            continue;
        }
        special.join_count = 0;
        special.load_count = 0;
        int dest_count = Reach(map, pos, unit, dests, &special);
        for (int j = 0; j < special.join_count; j++) {
            Push(list, MoveKind::Join, unit.tile, special.joins[j], special.joins[j]);
        }
        for (int j = 0; j < special.load_count; j++) {
            Push(list, MoveKind::Load, unit.tile, special.loads[j], special.loads[j]);
        }
        for (int d = 0; d < dest_count; d++) {
            int dest = dests[d];
            if (unit.cargo_count > 0) {
                PushUnloads(map, pos, unit, dest, list);
            }
            PushAttacksFrom(map, pos, unit, unit.tile, dest, list);
            int prop = map.property_at[dest];
            if (prop != kNoProperty && pos.prop_owner[prop] != pos.side &&
                Spec(unit.type).can_capture) {
                Push(list, MoveKind::Capture, unit.tile, dest, dest);
            }
            Push(list, MoveKind::Wait, unit.tile, dest, dest);
        }
    }
    if (map.production) {
        PushBuilds(map, pos, list);
    }
    Push(list, MoveKind::EndTurn, 0, 0, 0);
}

bool IsLegal(const Map& map, const Position& pos, const Move& move) {
    static thread_local MoveList list;
    GenerateMoves(map, pos, &list);
    for (int i = 0; i < list.count; i++) {
        if (list.moves[i] == move) {
            return true;
        }
    }
    return false;
}

} // namespace shoal
