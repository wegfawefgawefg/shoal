#include "search/table.hpp"

namespace shoal {

void TableResize(Table* table, int log2_entries) {
    std::size_t count = std::size_t{1} << log2_entries;
    table->entries.assign(count, TableEntry{});
    table->mask = count - 1;
    table->age = 0;
}

void TableClear(Table* table) {
    for (TableEntry& entry : table->entries) {
        entry = TableEntry{};
    }
    table->age = 0;
}

const TableEntry* TableProbe(const Table& table, std::uint64_t key) {
    const TableEntry& entry = table.entries[key & table.mask];
    if (entry.bound == Bound::None || entry.key != key) {
        return nullptr;
    }
    return &entry;
}

void TableStore(Table* table, std::uint64_t key, int score, int depth, Bound bound,
                const Move& best) {
    TableEntry& entry = table->entries[key & table->mask];
    bool same = entry.key == key;
    bool stale = entry.age != table->age;
    if (!same && !stale && entry.bound != Bound::None && entry.depth > depth) {
        return;
    }
    entry.key = key;
    entry.score = score;
    entry.depth = static_cast<std::int16_t>(depth);
    entry.bound = bound;
    entry.age = table->age;
    entry.best = best;
}

} // namespace shoal
