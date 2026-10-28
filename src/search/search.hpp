#pragma once

#include "game/position.hpp"
#include "search/table.hpp"

#include <chrono>
#include <cstdint>
#include <deque>
#include <vector>

namespace shoal {

struct SearchLimits {
    int max_depth = 64;       // plies; one unit action is one ply
    double seconds = 0;       // 0 means no time limit
    std::uint64_t nodes = 0;  // 0 means no node limit
    bool solve = false;       // search to the end of the game; scores are -1, 0, 1
    bool quiet = false;       // no per-depth printing
};

struct SearchResult {
    Move best;
    int score = 0; // from the view of the side to move
    int depth = 0;
    bool complete = false; // false when a limit stopped the last iteration
    std::uint64_t nodes = 0;
    double seconds = 0;
    std::vector<Move> pv;
};

// moves and their order scores for one ply of the search
struct PlyScratch {
    MoveList list;
    int scores[kMaxMoves];
};

struct Searcher {
    const Map* map = nullptr;
    Table table;
    std::deque<PlyScratch> plies; // a deque: growing it never moves the plies in use
    std::vector<std::int32_t> history; // quiet-move cutoffs by from and to tile
    std::uint64_t nodes = 0;
    std::uint64_t node_limit = 0;
    std::chrono::steady_clock::time_point deadline;
    bool timed = false;
    bool stopped = false;
    bool solve = false;
};

void SearcherInit(Searcher* searcher, const Map& map, int table_log2);
SearchResult Search(Searcher* searcher, const Position& root, const SearchLimits& limits);

} // namespace shoal
