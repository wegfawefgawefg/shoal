#pragma once

#include "game/position.hpp"

#include <string>

namespace shoal {

// terminal picture of a position: player 0 units upper case, player 1 lower case
std::string BoardText(const Map& map, const Position& pos);
char UnitLetter(UnitType type);

} // namespace shoal
