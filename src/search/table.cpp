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

namespace {

std::size_t BucketStart(const Table& table, std::uint64_t key) {
    return static_cast<std::size_t>(key & table.mask) & ~static_cast<std::size_t>(kBucketSize - 1);
}

std::uint8_t Log2(std::uint64_t nodes) {
    std::uint8_t bits = 0;
    while (nodes > 1 && bits < 63) {
        nodes >>= 1;
        bits++;
    }
    return bits;
}

// how much an entry is worth keeping: entries from older searches are worth nothing, then
// depth, then the work it took
int KeepValue(const Table& table, const TableEntry& entry) {
    if (entry.bound == Bound::None || entry.age != table.age) {
        return -1;
    }
    return entry.depth * 64 + entry.work;
}

} // namespace

const TableEntry* TableProbe(const Table& table, std::uint64_t key) {
    std::size_t start = BucketStart(table, key);
    for (int i = 0; i < kBucketSize; i++) {
        const TableEntry& entry = table.entries[start + static_cast<std::size_t>(i)];
        if (entry.bound != Bound::None && entry.key == key) {
            return &entry;
        }
    }
    return nullptr;
}

void TableStore(Table* table, std::uint64_t key, int score, int depth, Bound bound,
                const Move& best, std::uint64_t nodes) {
    std::size_t start = BucketStart(*table, key);
    TableEntry* slot = nullptr;
    for (int i = 0; i < kBucketSize && slot == nullptr; i++) {
        TableEntry& entry = table->entries[start + static_cast<std::size_t>(i)];
        if (entry.bound != Bound::None && entry.key == key) {
            slot = &entry;
        }
    }
    if (slot == nullptr) {
        slot = &table->entries[start];
        for (int i = 1; i < kBucketSize; i++) {
            TableEntry& entry = table->entries[start + static_cast<std::size_t>(i)];
            if (KeepValue(*table, entry) < KeepValue(*table, *slot)) {
                slot = &entry;
            }
        }
    }
    TableEntry& entry = *slot;
    entry.work = Log2(nodes);
    entry.key = key;
    entry.score = score;
    entry.depth = static_cast<std::int16_t>(depth);
    entry.bound = bound;
    entry.age = table->age;
    entry.best = best;
}

} // namespace shoal
