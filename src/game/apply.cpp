#include "game/position.hpp"

#include <cstring>

namespace shoal {

namespace {

void RemoveUnit(Position* pos, int index) {
    Unit& gone = pos->units[index];
    pos->unit_at[gone.tile] = kNoUnit;
    int last = pos->unit_count - 1;
    if (index != last) {
        pos->units[index] = pos->units[last];
        pos->unit_at[pos->units[index].tile] = static_cast<std::uint8_t>(index);
    }
    pos->unit_count = static_cast<std::uint8_t>(last);
}

// a unit standing on a property it was capturing is gone or has left: the capture resets
void ResetCaptureAt(const Map& map, Position* pos, int tile) {
    int prop = map.property_at[tile];
    if (prop != kNoProperty) {
        pos->capture[prop] = kCapturePoints;
    }
}

// fires one shot and spends ammo when the primary weapon is used
int Fire(const Map& map, Unit* attacker, const Unit& defender, int defender_tile) {
    bool primary = PrimaryDamage(attacker->type, defender.type) > 0 && attacker->ammo > 0;
    int damage = AttackDamage(map, *attacker, defender, defender_tile);
    if (primary) {
        attacker->ammo--;
    }
    return damage;
}

void KillCheck(const Map& map, Position* pos, int index) {
    Unit& unit = pos->units[index];
    if (unit.hp > 0) {
        return;
    }
    int owner = unit.owner;
    ResetCaptureAt(map, pos, unit.tile);
    RemoveUnit(pos, index);
    if (UnitCount(*pos, owner) == 0 && pos->result == Result::Ongoing) {
        pos->result = owner == 0 ? Result::Win1 : Result::Win0;
    }
}

void Resolve(const Map& map, Position* pos, int attacker_tile, int defender_tile) {
    Unit& attacker = pos->units[pos->unit_at[attacker_tile]];
    Unit& defender = pos->units[pos->unit_at[defender_tile]];
    int damage = Fire(map, &attacker, defender, defender_tile);
    defender.hp = static_cast<std::uint8_t>(damage >= defender.hp ? 0 : defender.hp - damage);
    if (defender.hp == 0) {
        KillCheck(map, pos, pos->unit_at[defender_tile]);
        return;
    }
    // direct defenders strike back with their new hit points
    if (IsIndirect(defender.type) || Distance(map, attacker_tile, defender_tile) != 1) {
        return;
    }
    if (!CanHit(defender, attacker)) {
        return;
    }
    int counter = Fire(map, &defender, attacker, attacker_tile);
    attacker.hp = static_cast<std::uint8_t>(counter >= attacker.hp ? 0 : attacker.hp - counter);
    KillCheck(map, pos, pos->unit_at[attacker_tile]);
}

void Repair(const Map& map, Position* pos, Unit* unit) {
    int prop = map.property_at[unit->tile];
    if (prop == kNoProperty || pos->prop_owner[prop] != unit->owner) {
        return;
    }
    if (!RepairsUnit(map.tiles[unit->tile], unit->type)) {
        return;
    }
    unit->ammo = static_cast<std::uint8_t>(Spec(unit->type).ammo);
    unit->fuel = static_cast<std::uint8_t>(Spec(unit->type).fuel);
    // up to two board hit points, paid at a tenth of the unit cost each
    int tenth = Spec(unit->type).cost / 10;
    for (int heal = 20; heal >= 10; heal -= 10) {
        int hp = unit->hp + heal > 100 ? 100 : unit->hp + heal;
        int gained = DisplayHp(hp) - DisplayHp(unit->hp);
        int price = gained * tenth;
        if (price > pos->funds[unit->owner]) {
            continue;
        }
        pos->funds[unit->owner] -= price;
        unit->hp = static_cast<std::uint8_t>(hp);
        return;
    }
}

// air and sea units burn their daily fuel, from day 2 on; those left empty go down with
// their cargo before anything can resupply them
void BurnFuel(const Map& map, Position* pos) {
    if (pos->day < 2) {
        return;
    }
    for (int i = pos->unit_count - 1; i >= 0; i--) {
        Unit& unit = pos->units[i];
        int burn = unit.dived ? kSubDivedFuelPerDay : Spec(unit.type).fuel_per_day;
        if (unit.owner != pos->side || burn == 0) {
            continue;
        }
        unit.fuel = static_cast<std::uint8_t>(unit.fuel > burn ? unit.fuel - burn : 0);
        if (unit.fuel == 0) {
            unit.hp = 0;
            KillCheck(map, pos, i); // moves the last unit, already burnt, into i
        }
    }
}

void StartTurn(const Map& map, Position* pos) {
    int side = pos->side;
    pos->funds[side] += map.funds_per_property * PropertyCount(*pos, map, side);
    BurnFuel(map, pos);
    for (int i = 0; i < pos->unit_count; i++) {
        Unit& unit = pos->units[i];
        unit.done = 0;
        if (unit.owner != side) {
            continue;
        }
        Repair(map, pos, &unit);
        // a cruiser rearms and refuels the copters aboard
        for (int c = 0; c < unit.cargo_count && unit.type == UnitType::Cruiser; c++) {
            Cargo& cargo = unit.cargo[c];
            cargo.ammo = static_cast<std::uint8_t>(Spec(cargo.type).ammo);
            cargo.fuel = static_cast<std::uint8_t>(Spec(cargo.type).fuel);
        }
    }
    // apcs refill the ammo and fuel of their own side's units next to them
    for (int i = 0; i < pos->unit_count; i++) {
        const Unit& apc = pos->units[i];
        if (apc.owner != side || apc.type != UnitType::Apc) {
            continue;
        }
        int around[4];
        int count = Neighbours(map, apc.tile, around);
        for (int n = 0; n < count; n++) {
            std::uint8_t index = pos->unit_at[around[n]];
            if (index != kNoUnit && pos->units[index].owner == side) {
                Unit& unit = pos->units[index];
                unit.ammo = static_cast<std::uint8_t>(Spec(unit.type).ammo);
                unit.fuel = static_cast<std::uint8_t>(Spec(unit.type).fuel);
            }
        }
    }
}

void ScoreDayLimit(const Map& map, Position* pos) {
    int mine = PropertyCount(*pos, map, 0);
    int theirs = PropertyCount(*pos, map, 1);
    if (mine > theirs) {
        pos->result = Result::Win0;
    } else if (theirs > mine) {
        pos->result = Result::Win1;
    } else {
        pos->result = Result::Draw;
    }
}

void EndTurn(const Map& map, Position* pos) {
    if (pos->side == 1) {
        if (pos->day >= map.day_limit) {
            ScoreDayLimit(map, pos);
            return;
        }
        pos->day++;
    }
    pos->side ^= 1;
    StartTurn(map, pos);
}

// the mover pays fuel for the path it takes this move
void SpendFuel(const Map& map, const Position& pos, const Move& move, Unit* unit) {
    int points = MovePoints(map, pos, move);
    unit->fuel = static_cast<std::uint8_t>(unit->fuel > points ? unit->fuel - points : 0);
}

// the mover merges into the unit on `to`: hit points add up to 10 on the board, the board hp
// past 10 come back as funds at a tenth of the unit's cost each, ammo and fuel add up to
// full; the merged unit has acted and keeps any capture it was making
void Join(const Map& map, Position* pos, const Move& move) {
    int mover_index = pos->unit_at[move.from];
    Unit mover = pos->units[mover_index];
    SpendFuel(map, *pos, move, &mover);
    Unit& target = pos->units[pos->unit_at[move.to]];
    int shown = DisplayHp(mover.hp) + DisplayHp(target.hp);
    if (shown > 10) {
        pos->funds[mover.owner] += (shown - 10) * Spec(mover.type).cost / 10;
    }
    int hp = mover.hp + target.hp;
    target.hp = static_cast<std::uint8_t>(hp > 100 ? 100 : hp);
    int ammo = mover.ammo + target.ammo;
    int full = Spec(mover.type).ammo;
    target.ammo = static_cast<std::uint8_t>(ammo > full ? full : ammo);
    int fuel = mover.fuel + target.fuel;
    int tank = Spec(mover.type).fuel;
    target.fuel = static_cast<std::uint8_t>(fuel > tank ? tank : fuel);
    target.done = 1;
    ResetCaptureAt(map, pos, move.from);
    RemoveUnit(pos, mover_index);
}

// the mover leaves the board and rides in the transport on `to`
void Load(const Map& map, Position* pos, const Move& move) {
    int mover_index = pos->unit_at[move.from];
    Unit mover = pos->units[mover_index];
    SpendFuel(map, *pos, move, &mover);
    Unit& transport = pos->units[pos->unit_at[move.to]];
    Cargo& cargo = transport.cargo[transport.cargo_count++];
    cargo.type = mover.type;
    cargo.hp = mover.hp;
    cargo.ammo = mover.ammo;
    cargo.fuel = mover.fuel;
    ResetCaptureAt(map, pos, move.from);
    RemoveUnit(pos, mover_index);
}

// the transport moves, then puts one cargo on the drop tile; the cargo has acted, the
// transport may still drop the rest where it stands
void Unload(const Map& map, Position* pos, const Move& move) {
    int index = pos->unit_at[move.from];
    SpendFuel(map, *pos, move, &pos->units[index]);
    if (move.to != move.from) {
        pos->unit_at[move.from] = kNoUnit;
        pos->unit_at[move.to] = static_cast<std::uint8_t>(index);
        pos->units[index].tile = move.to;
    }
    Unit& transport = pos->units[index];
    Cargo cargo = transport.cargo[move.slot];
    for (int i = move.slot; i + 1 < transport.cargo_count; i++) {
        transport.cargo[i] = transport.cargo[i + 1];
    }
    transport.cargo_count--;
    transport.cargo[transport.cargo_count] = Cargo{};
    transport.done = transport.cargo_count > 0 ? kDropping : 1;
    int dropped = pos->unit_count++;
    Unit& unit = pos->units[dropped];
    unit = Unit{};
    unit.type = cargo.type;
    unit.owner = transport.owner;
    unit.hp = cargo.hp;
    unit.ammo = cargo.ammo;
    unit.fuel = cargo.fuel;
    unit.tile = move.target;
    unit.done = 1;
    pos->unit_at[move.target] = static_cast<std::uint8_t>(dropped);
}

} // namespace

int AttackDamage(const Map& map, const Unit& attacker, const Unit& defender, int defender_tile) {
    int base = PrimaryDamage(attacker.type, defender.type);
    if (base == 0 || attacker.ammo == 0) {
        base = SecondaryDamage(attacker.type, defender.type);
    }
    if (base == 0) {
        return 0;
    }
    // AWBW formula without luck or COs, in exact integers:
    // base * (attacker hp / 10) * (100 - stars * defender hp) / 100; air units get no cover
    int stars = Spec(map.tiles[defender_tile]).defense;
    if (DomainOf(defender.type) == Domain::Air) {
        stars = 0;
    }
    int cover = 100 - stars * DisplayHp(defender.hp);
    return base * DisplayHp(attacker.hp) * cover / 1000;
}

int UnitCount(const Position& pos, int owner) {
    int count = 0;
    for (int i = 0; i < pos.unit_count; i++) {
        if (pos.units[i].owner == owner) {
            count++;
        }
    }
    return count;
}

int PropertyCount(const Position& pos, const Map& map, int owner) {
    int count = 0;
    for (int prop = 0; prop < map.property_count; prop++) {
        if (pos.prop_owner[prop] == owner) {
            count++;
        }
    }
    return count;
}

void SetupStart(const Map& map, Position* pos) {
    *pos = Position{};
    std::memset(pos->unit_at, kNoUnit, sizeof(pos->unit_at));
    for (int prop = 0; prop < map.property_count; prop++) {
        pos->prop_owner[prop] = map.start_owner[prop];
        pos->capture[prop] = kCapturePoints;
    }
    for (const StartUnit& start : map.start_units) {
        if (pos->unit_count >= kMaxUnits) {
            break;
        }
        int index = pos->unit_count++;
        Unit& unit = pos->units[index];
        unit.type = start.type;
        unit.owner = static_cast<std::uint8_t>(start.owner);
        unit.hp = static_cast<std::uint8_t>(start.hp);
        unit.ammo = static_cast<std::uint8_t>(Spec(start.type).ammo);
        unit.fuel = static_cast<std::uint8_t>(start.fuel < 0 ? Spec(start.type).fuel : start.fuel);
        unit.tile = static_cast<std::uint16_t>(start.tile);
        pos->unit_at[start.tile] = static_cast<std::uint8_t>(index);
    }
    pos->funds[0] = map.start_funds[0];
    pos->funds[1] = map.start_funds[1];
    pos->side = 0;
    pos->day = 1;
    StartTurn(map, pos);
    pos->hash = ComputeHash(map, *pos);
}

void ApplyMove(const Map& map, Position* pos, const Move& move) {
    switch (move.kind) {
    case MoveKind::EndTurn:
        EndTurn(map, pos);
        break;
    case MoveKind::Build: {
        int index = pos->unit_count++;
        Unit& unit = pos->units[index];
        unit = Unit{};
        unit.type = move.build;
        unit.owner = pos->side;
        unit.hp = 100;
        unit.ammo = static_cast<std::uint8_t>(Spec(move.build).ammo);
        unit.fuel = static_cast<std::uint8_t>(Spec(move.build).fuel);
        unit.tile = move.to;
        unit.done = 1;
        pos->unit_at[move.to] = static_cast<std::uint8_t>(index);
        pos->funds[pos->side] -= Spec(move.build).cost;
        break;
    }
    case MoveKind::Join:
        Join(map, pos, move);
        break;
    case MoveKind::Load:
        Load(map, pos, move);
        break;
    case MoveKind::Unload:
        Unload(map, pos, move);
        break;
    case MoveKind::Wait:
    case MoveKind::Attack:
    case MoveKind::Capture:
    case MoveKind::Dive:
    case MoveKind::Surface: {
        int index = pos->unit_at[move.from];
        Unit& unit = pos->units[index];
        SpendFuel(map, *pos, move, &unit);
        if (move.kind == MoveKind::Dive || move.kind == MoveKind::Surface) {
            unit.dived = move.kind == MoveKind::Dive ? 1 : 0;
        }
        if (move.to != move.from) {
            ResetCaptureAt(map, pos, move.from);
            pos->unit_at[move.from] = kNoUnit;
            pos->unit_at[move.to] = static_cast<std::uint8_t>(index);
            unit.tile = move.to;
        }
        unit.done = 1;
        if (move.kind == MoveKind::Attack) {
            Resolve(map, pos, move.to, move.target);
        } else if (move.kind == MoveKind::Capture) {
            int prop = map.property_at[move.to];
            int left = pos->capture[prop] - DisplayHp(unit.hp);
            if (left > 0) {
                pos->capture[prop] = static_cast<std::uint8_t>(left);
                break;
            }
            pos->capture[prop] = kCapturePoints;
            pos->prop_owner[prop] = pos->side;
            if (map.tiles[move.to] == Terrain::Hq) {
                pos->result = pos->side == 0 ? Result::Win0 : Result::Win1;
            }
        }
        break;
    }
    }
    pos->hash = ComputeHash(map, *pos);
}

} // namespace shoal
