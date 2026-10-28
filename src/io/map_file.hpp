#pragma once

#include "game/position.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace shoal {

// a played game: the map it started from and every move in order
struct GameRecord {
    Map map;
    std::vector<Move> moves;
    std::vector<int> evals; // optional, one per move, from the side that moved; empty if none
};

bool LoadMapText(std::string_view text, Map* map, std::string* error);
bool LoadMapFile(const std::string& path, Map* map, std::string* error);
std::string WriteMapText(const Map& map);

bool LoadGameText(std::string_view text, GameRecord* game, std::string* error);
bool LoadGameFile(const std::string& path, GameRecord* game, std::string* error);
std::string WriteGameText(const GameRecord& game);

bool ReadWholeFile(const std::string& path, std::string* out);
bool WriteWholeFile(const std::string& path, std::string_view text);

} // namespace shoal
