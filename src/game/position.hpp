#pragma once

#include "game/map.hpp"

#include <cstdint>
#include <string>

namespace shoal {

constexpr int kMaxUnits = 128; // both sides together; unit_at holds indices below 0xff
constexpr std::uint8_t kNoUnit = 0xff;
constexpr int kCapturePoints = 20;
constexpr int kMaxCargo = 2;

// a unit riding in a transport; it has no tile of its own
struct Cargo {
    UnitType type = UnitType::Infantry;
    std::uint8_t hp = 100;
    std::uint8_t ammo = 0;
    std::uint8_t fuel = 0;
};

struct Unit {
    UnitType type = UnitType::Infantry;
    std::uint8_t owner = 0;
    std::uint8_t hp = 100; // internal hit points 1..100; the board shows (hp + 9) / 10
    std::uint8_t ammo = 0;
    std::uint16_t tile = 0;
    std::uint8_t done = 0; // acted this turn; kDropping: dropped cargo, may drop the rest
    std::uint8_t cargo_count = 0;
    std::uint8_t fuel = 0;
    std::uint8_t dived = 0; // a sub under water
    Cargo cargo[kMaxCargo];
};

// a transport that has dropped one cargo and not moved since may drop its other cargo
constexpr std::uint8_t kDropping = 2;

inline int DisplayHp(int hp) {
    return (hp + 9) / 10;
}

// a quarter tank or less
inline bool LowFuel(const Unit& unit) {
    return unit.fuel * 4 <= Spec(unit.type).fuel;
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
    Join,    // move onto a hurt friendly unit of the same type and merge into it
    Load,    // move into a friendly transport on `to`
    Unload,  // the transport moves to `to` and drops cargo `slot` on `target`
    Dive,    // a sub moves to `to` and dives
    Surface, // a dived sub moves to `to` and comes up
};

// Move::points when the move did not come from GenerateMoves
constexpr std::uint8_t kUnknownPoints = 0xff;

// one unit action, or the end of the turn; units are named by the tile they start on
struct Move {
    MoveKind kind = MoveKind::EndTurn;
    UnitType build = UnitType::Infantry;
    std::uint16_t from = 0;
    std::uint16_t to = 0;
    std::uint16_t target = 0;
    std::uint8_t slot = 0; // which cargo an unload drops
    // move points (and fuel) the unit spends getting to `to`; GenerateMoves fills it, other
    // moves leave kUnknownPoints and ApplyMove works it out. Not part of a move's identity
    std::uint8_t points = kUnknownPoints;
};

// a move with the same identity as `a`; ignores `points`
inline bool operator==(const Move& a, const Move& b) {
    return a.kind == b.kind && a.build == b.build && a.from == b.from && a.to == b.to &&
           a.target == b.target && a.slot == b.slot;
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

// tiles a unit can end its move on (its own tile included); friends can be passed, enemies
// block; returns the count
int ReachableTiles(const Map& map, const Position& pos, const Unit& unit, int out[kMaxTiles]);
// move points a unit spends on its cheapest path to `to` (0 to stay), or -1 if it cannot get
// there; the same limits as ReachableTiles
int PathPoints(const Map& map, const Position& pos, const Unit& unit, int to);
// the points a move spends: move.points when known, else PathPoints
int MovePoints(const Map& map, const Position& pos, const Move& move);
// a unit may join `target`: same owner and type, a different unit, target shows < 10 hp, and
// neither carries anything
bool CanJoin(const Unit& unit, const Unit& target);
// a unit may load into `transport`: a friendly transport that carries its type and has room
bool CanLoad(const Unit& unit, const Unit& transport);

// damage percent a full attack would do; 0 when the attacker cannot hit the defender
int AttackDamage(const Map& map, const Unit& attacker, const Unit& defender, int defender_tile);

int UnitCount(const Position& pos, int owner);
int PropertyCount(const Position& pos, const Map& map, int owner);

std::uint64_t ComputeHash(const Map& map, const Position& pos);
// one unit's share of the hash; the hash xors these with the board terms
std::uint64_t UnitHashKey(const Unit& unit);

// whether `attacker` can fire on `defender` at all now: a weapon that hits it with ammo to
// spare, and a dived sub only by cruisers and subs
bool CanHit(const Unit& attacker, const Unit& defender);

// "a2 b2", "a2 b2 x c2", "a2 a2 cap", "a2 b2 join", "a2 b2 load", "b2 c2 drop c3",
// "b2 c2 drop c3 2" (second cargo), "a1 b1 dive", "a1 b1 surface", "b1 build inf", "end"
std::string MoveText(const Map& map, const Move& move);
bool ParseMoveText(const Map& map, std::string_view text, Move* move);

} // namespace shoal
