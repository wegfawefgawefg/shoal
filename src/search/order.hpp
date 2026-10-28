#pragma once

#include "game/position.hpp"

#include <cstdint>

namespace shoal {

// order hints: table move, then wins, captures that finish, attacks by value, history
void ScoreMoves(const Map& map, const Position& pos, const MoveList& list,
                const Move* table_move, const std::int32_t* history, int* scores);
// moves the best remaining move to index i (selection sort, so cutoffs skip the rest)
void PickNext(MoveList* list, int* scores, int i);
void HistoryReward(std::int32_t* history, const Move& move, int depth);

} // namespace shoal
