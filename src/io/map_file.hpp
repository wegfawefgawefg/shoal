#pragma once

#include "game/position.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace shoal {

// how bad a played turn was next to the engine's best (`shoal analyse`)
enum class TurnMark : std::uint8_t {
    None,
    Inaccuracy,
    Mistake,
    Blunder,
};

const char* TurnMarkName(TurnMark mark); // "none", "inaccuracy", "mistake", "blunder"
bool TurnMarkFromName(std::string_view name, TurnMark* mark);
const char* TurnMarkSymbol(TurnMark mark); // "", "?!", "?", "??"

// the engine's view of one turn start
struct TurnAnalysis {
    int ply = 0;   // moves played before the turn starts
    int score = 0; // from the view of the side to move there
    int loss = 0;  // what the turn actually played cost that side; 0 when unknown
    TurnMark mark = TurnMark::None;
    std::vector<Move> best; // the engine's turn from here, without the final end
};

// a played game: the map it started from and every move in order
struct GameRecord {
    Map map;
    std::vector<Move> moves;
    std::vector<int> evals; // optional, one per move, from the side that moved; empty if none
    std::vector<TurnAnalysis> analysis; // optional, in ply order
};

std::string TurnMovesText(const Map& map,
                          const std::vector<Move>& moves); // "b1 b2, c3 build inf"; "" for none

bool LoadMapText(std::string_view text, Map* map, std::string* error);
bool LoadMapFile(const std::string& path, Map* map, std::string* error);
std::string WriteMapText(const Map& map);

bool LoadGameText(std::string_view text, GameRecord* game, std::string* error);
bool LoadGameFile(const std::string& path, GameRecord* game, std::string* error);
std::string WriteGameText(const GameRecord& game);

bool ReadWholeFile(const std::string& path, std::string* out);
bool WriteWholeFile(const std::string& path, std::string_view text);

} // namespace shoal
