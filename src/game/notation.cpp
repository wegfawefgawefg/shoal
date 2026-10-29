#include "game/position.hpp"

#include <vector>

namespace shoal {

namespace {

std::uint64_t Mix(std::uint64_t x) {
    // splitmix64 finaliser
    x += 0x9e3779b97f4a7c15ull;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
    return x ^ (x >> 31);
}

std::vector<std::string_view> SplitWords(std::string_view text) {
    std::vector<std::string_view> words;
    std::size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && text[i] == ' ') {
            i++;
        }
        std::size_t start = i;
        while (i < text.size() && text[i] != ' ') {
            i++;
        }
        if (i > start) {
            words.push_back(text.substr(start, i - start));
        }
    }
    return words;
}

} // namespace

std::uint64_t UnitHashKey(const Unit& unit) {
    std::uint64_t key = static_cast<std::uint64_t>(unit.tile);
    key = key << 8 | static_cast<std::uint64_t>(unit.type);
    key = key << 8 | unit.owner;
    key = key << 8 | unit.hp;
    key = key << 8 | unit.ammo;
    key = key << 8 | unit.done;
    std::uint64_t hash = Mix(key);
    for (int i = 0; i < unit.cargo_count; i++) {
        const Cargo& cargo = unit.cargo[i];
        std::uint64_t cargo_key = 6ull << 60 | static_cast<std::uint64_t>(i) << 40 |
                                  static_cast<std::uint64_t>(unit.tile) << 24 |
                                  static_cast<std::uint64_t>(cargo.type) << 16 |
                                  static_cast<std::uint64_t>(cargo.hp) << 8 | cargo.ammo;
        hash ^= Mix(cargo_key);
    }
    return hash;
}

std::uint64_t ComputeHash(const Map& map, const Position& pos) {
    // units xor together so their order in the array does not matter
    std::uint64_t hash = 0;
    for (int i = 0; i < pos.unit_count; i++) {
        hash ^= UnitHashKey(pos.units[i]);
    }
    for (int prop = 0; prop < map.property_count; prop++) {
        std::uint64_t key = 1ull << 60 | static_cast<std::uint64_t>(prop) << 16;
        key |= static_cast<std::uint64_t>(pos.prop_owner[prop]) << 8 | pos.capture[prop];
        hash ^= Mix(key);
    }
    hash ^= Mix(2ull << 60 | static_cast<std::uint64_t>(pos.day) << 8 | pos.side);
    hash ^= Mix(3ull << 60 | static_cast<std::uint32_t>(pos.funds[0]));
    hash ^= Mix(4ull << 60 | static_cast<std::uint32_t>(pos.funds[1]));
    hash ^= Mix(5ull << 60 | static_cast<std::uint64_t>(pos.result));
    return hash;
}

std::string MoveText(const Map& map, const Move& move) {
    switch (move.kind) {
    case MoveKind::EndTurn:
        return "end";
    case MoveKind::Build:
        return TileName(map, move.to) + " build " + std::string(Spec(move.build).short_name);
    case MoveKind::Wait:
        return TileName(map, move.from) + " " + TileName(map, move.to);
    case MoveKind::Attack:
        return TileName(map, move.from) + " " + TileName(map, move.to) + " x " +
               TileName(map, move.target);
    case MoveKind::Capture:
        return TileName(map, move.from) + " " + TileName(map, move.to) + " cap";
    case MoveKind::Join:
        return TileName(map, move.from) + " " + TileName(map, move.to) + " join";
    case MoveKind::Load:
        return TileName(map, move.from) + " " + TileName(map, move.to) + " load";
    case MoveKind::Unload: {
        std::string text = TileName(map, move.from) + " " + TileName(map, move.to) + " drop " +
                           TileName(map, move.target);
        if (move.slot > 0) {
            text += " " + std::to_string(move.slot + 1);
        }
        return text;
    }
    }
    return "?";
}

bool ParseMoveText(const Map& map, std::string_view text, Move* move) {
    std::vector<std::string_view> words = SplitWords(text);
    *move = Move{};
    if (words.size() == 1 && words[0] == "end") {
        move->kind = MoveKind::EndTurn;
        return true;
    }
    int from = 0;
    if (words.size() < 2 || !ParseTileName(map, words[0], &from)) {
        return false;
    }
    move->from = static_cast<std::uint16_t>(from);
    if (words.size() == 3 && words[1] == "build") {
        UnitType type;
        if (!UnitTypeFromShortName(words[2], &type)) {
            return false;
        }
        move->kind = MoveKind::Build;
        move->build = type;
        move->to = move->from;
        move->target = move->from;
        return true;
    }
    int to = 0;
    if (!ParseTileName(map, words[1], &to)) {
        return false;
    }
    move->to = static_cast<std::uint16_t>(to);
    move->target = move->to;
    if (words.size() == 2) {
        move->kind = MoveKind::Wait;
        return true;
    }
    if (words.size() == 3 && words[2] == "cap") {
        move->kind = MoveKind::Capture;
        return true;
    }
    if (words.size() == 3 && words[2] == "join") {
        move->kind = MoveKind::Join;
        return true;
    }
    if (words.size() == 3 && words[2] == "load") {
        move->kind = MoveKind::Load;
        return true;
    }
    int drop = 0;
    if ((words.size() == 4 || words.size() == 5) && words[2] == "drop" &&
        ParseTileName(map, words[3], &drop)) {
        move->kind = MoveKind::Unload;
        move->target = static_cast<std::uint16_t>(drop);
        if (words.size() == 5) {
            if (words[4] != "1" && words[4] != "2") {
                return false;
            }
            move->slot = static_cast<std::uint8_t>(words[4][0] - '1');
        }
        return true;
    }
    int target = 0;
    if (words.size() == 4 && words[2] == "x" && ParseTileName(map, words[3], &target)) {
        move->kind = MoveKind::Attack;
        move->target = static_cast<std::uint16_t>(target);
        return true;
    }
    return false;
}

} // namespace shoal
