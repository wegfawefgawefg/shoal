#pragma once

#include "game/position.hpp"

#include <string>

namespace shoal {

// the board agrees with itself: tile index, unit list, hash, hit points, fuel, cargo, terrain,
// funds; on failure `why` says what broke
bool Consistent(const Map& map, const Position& pos, std::string* why);

} // namespace shoal
