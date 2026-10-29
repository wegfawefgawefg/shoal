#pragma once

// small helpers the rules tests share: maps from text, moves from notation, units by tile

#include "game/position.hpp"
#include "io/map_file.hpp"

#include <cstdio>
#include <memory>
#include <string>

namespace shoal {

inline std::unique_ptr<Map> MapFrom(const char* text) {
    auto map = std::make_unique<Map>();
    std::string error;
    if (!LoadMapText(text, map.get(), &error)) {
        std::printf("map error: %s\n", error.c_str());
    }
    return map;
}

inline Move M(const Map& map, const char* text) {
    Move move;
    ParseMoveText(map, text, &move);
    return move;
}

inline bool Play(const Map& map, Position* pos, const char* text) {
    Move move = M(map, text);
    if (!IsLegal(map, *pos, move)) {
        std::printf("illegal: %s\n", text);
        return false;
    }
    ApplyMove(map, pos, move);
    return true;
}

inline const Unit* UnitOn(const Map& map, const Position& pos, const char* tile_name) {
    int tile = 0;
    ParseTileName(map, tile_name, &tile);
    if (pos.unit_at[tile] == kNoUnit) {
        return nullptr;
    }
    return &pos.units[pos.unit_at[tile]];
}

inline Unit MakeUnit(UnitType type, int owner, int hp, int ammo) {
    Unit unit;
    unit.type = type;
    unit.owner = static_cast<std::uint8_t>(owner);
    unit.hp = static_cast<std::uint8_t>(hp);
    unit.ammo = static_cast<std::uint8_t>(ammo);
    return unit;
}

} // namespace shoal
