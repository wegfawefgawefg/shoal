#include "search/order.hpp"

namespace shoal {

namespace {

constexpr int kTableMoveScore = 2000000000;
constexpr int kWinningCapture = 1500000000;
constexpr int kTacticalBase = 1000000000;

int AttackScore(const Map& map, const Position& pos, const Move& move) {
    const Unit& attacker = pos.units[pos.unit_at[move.from]];
    const Unit& defender = pos.units[pos.unit_at[move.target]];
    int damage = AttackDamage(map, attacker, defender, move.target);
    if (damage >= defender.hp) {
        // kills come first, the dearer the better
        return kTacticalBase + 200000 + Spec(defender.type).cost;
    }
    int dealt = Spec(defender.type).cost * damage / 100;
    // the counter strikes with the defender's new hp from the attacker's new tile
    Unit hurt = defender;
    hurt.hp = static_cast<std::uint8_t>(defender.hp - damage);
    int taken = 0;
    if (!IsIndirect(defender.type) && Distance(map, move.to, move.target) == 1) {
        Unit moved = attacker;
        moved.tile = move.to;
        taken = Spec(attacker.type).cost * AttackDamage(map, hurt, moved, move.to) / 100;
    }
    return kTacticalBase + 100000 + dealt - taken;
}

int CaptureScore(const Map& map, const Position& pos, const Move& move) {
    int prop = map.property_at[move.to];
    const Unit& unit = pos.units[pos.unit_at[move.from]];
    int left = pos.capture[prop] - DisplayHp(unit.hp);
    if (move.to != move.from) {
        left = kCapturePoints - DisplayHp(unit.hp);
    }
    if (left <= 0 && map.tiles[move.to] == Terrain::Hq) {
        return kWinningCapture;
    }
    return kTacticalBase + (left <= 0 ? 150000 : 50000) + (kCapturePoints - left) * 100;
}

} // namespace

void ScoreMoves(const Map& map, const Position& pos, const MoveList& list,
                const Move* table_move, const std::int32_t* history, int* scores) {
    for (int i = 0; i < list.count; i++) {
        const Move& move = list.moves[i];
        int score = 0;
        switch (move.kind) {
        case MoveKind::Attack:
            score = AttackScore(map, pos, move);
            break;
        case MoveKind::Capture:
            score = CaptureScore(map, pos, move);
            break;
        case MoveKind::Build:
            score = kTacticalBase + Spec(move.build).cost / 10;
            break;
        case MoveKind::Wait:
            score = history[move.from * kMaxTiles + move.to];
            break;
        case MoveKind::Join:
            score = kTacticalBase;
            break;
        case MoveKind::EndTurn:
            score = -1;
            break;
        }
        if (table_move != nullptr && move == *table_move) {
            score = kTableMoveScore;
        }
        scores[i] = score;
    }
}

void PickNext(MoveList* list, int* scores, int i) {
    int best = i;
    for (int j = i + 1; j < list->count; j++) {
        if (scores[j] > scores[best]) {
            best = j;
        }
    }
    if (best == i) {
        return;
    }
    Move move = list->moves[i];
    list->moves[i] = list->moves[best];
    list->moves[best] = move;
    int score = scores[i];
    scores[i] = scores[best];
    scores[best] = score;
}

void HistoryReward(std::int32_t* history, const Move& move, int depth) {
    std::int32_t& slot = history[move.from * kMaxTiles + move.to];
    slot += depth * depth;
    if (slot > 1000000) {
        slot = 1000000;
    }
}

} // namespace shoal
