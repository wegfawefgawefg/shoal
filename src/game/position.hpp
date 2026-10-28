#pragma once

#include "game/map.hpp"

#include <cstdint>
#include <string>

namespace shoal {

constexpr int kMaxUnits = 64;
constexpr std::uint8_t kNoUnit = 0xff;
constexpr int kCapturePoints = 20;

struct Unit {
    UnitType type = UnitType::Infantry;
    std::uint8_t owner = 0;
    std::uint8_t hp = 100; // internal hit points 1..100; the board shows (hp + 9) / 10
    std::uint8_t ammo = 0;
    std::uint16_t tile = 0;
    std::uint8_t done = 0; // acted this turn
    std::uint8_t pad = 0;
};

inline int DisplayHp(int hp) {
    return (hp + 9) / 10;
}

enum class Result : std::uint8_t {
    Ongoing,
    Win0,
    Win1,
    Draw,
};

// everything that changes while a game is played; copied whole by search
struct Position {
    Unit units[kMaxUnits];
    std::uint8_t unit_count = 0;
    std::uint8_t side = 0; // whose turn it is
    Result result = Result::Ongoing;
    std::uint16_t day = 1;
    std::int32_t funds[2] = {0, 0};
    std::uint8_t unit_at[kMaxTiles];
    std::uint8_t prop_owner[kMaxProperties];
    std::uint8_t capture[kMaxProperties]; // points left; kCapturePoints when nobody is capturing
    std::uint64_t hash = 0;
};

enum class MoveKind : std::uint8_t {
    EndTurn,
    Wait,
    Attack,
    Capture,
    Build,
};

// one unit action, or the end of the turn; units are named by the tile they start on
struct Move {
    MoveKind kind = MoveKind::EndTurn;
    UnitType build = UnitType::Infantry;
    std::uint16_t from = 0;
    std::uint16_t to = 0;
    std::uint16_t target = 0;
};

inline bool operator==(const Move& a, const Move& b) {
    return a.kind == b.kind && a.build == b.build && a.from == b.from && a.to == b.to &&
           a.target == b.target;
}

constexpr int kMaxMoves = 8192;

struct MoveList {
    int count = 0;
    Move moves[kMaxMoves];
};

void SetupStart(const Map& map, Position* pos);

// all legal moves for the side to move; empty once the game is over
void GenerateMoves(const Map& map, const Position& pos, MoveList* list);
// plays a legal move; the caller guarantees legality
void ApplyMove(const Map& map, Position* pos, const Move& move);
bool IsLegal(const Map& map, const Position& pos, const Move& move);

// damage percent a full attack would do; 0 when the attacker cannot hit the defender
int AttackDamage(const Map& map, const Unit& attacker, const Unit& defender, int defender_tile);

int UnitCount(const Position& pos, int owner);
int PropertyCount(const Position& pos, const Map& map, int owner);

std::uint64_t ComputeHash(const Map& map, const Position& pos);

// "a2 b2", "a2 b2 x c2", "a2 a2 cap", "b1 build inf", "end"
std::string MoveText(const Map& map, const Move& move);
bool ParseMoveText(const Map& map, std::string_view text, Move* move);

} // namespace shoal
