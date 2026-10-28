#pragma once

#include "game/position.hpp"

#include <cstdint>
#include <vector>

namespace shoal {

enum class Bound : std::uint8_t {
    None,
    Exact,
    Lower, // the score is at least this
    Upper, // the score is at most this
};

struct TableEntry {
    std::uint64_t key = 0;
    std::int32_t score = 0;
    std::int16_t depth = -1;
    Bound bound = Bound::None;
    std::uint8_t age = 0;
    Move best;
};

// transposition table: one entry per slot, replaced when deeper or older
struct Table {
    std::vector<TableEntry> entries;
    std::uint64_t mask = 0;
    std::uint8_t age = 0;
};

void TableResize(Table* table, int log2_entries);
void TableClear(Table* table);
const TableEntry* TableProbe(const Table& table, std::uint64_t key);
void TableStore(Table* table, std::uint64_t key, int score, int depth, Bound bound,
                const Move& best);

} // namespace shoal
