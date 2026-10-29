#pragma once

#include "game/position.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace shoal {

// turns searched long and deep ahead of time, by the position at the start of each turn
struct Book {
    Map map;
    std::unordered_map<std::uint64_t, std::vector<Move>> turns;
    std::unordered_map<std::uint64_t, int> scores; // from the mover's view
};

bool LoadBook(const std::string& path, Book* book, std::string* error);
std::string WriteBookText(const Book& book);
// the book's turn for this position when it has one and every move is still legal
bool BookTurn(const Book& book, const Map& map, const Position& pos, std::vector<Move>* moves);

} // namespace shoal
