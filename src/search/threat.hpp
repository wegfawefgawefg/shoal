#pragma once

#include "game/position.hpp"

#include <cstdint>

namespace shoal {

// who can strike which tile next turn. Attackers are named by slot; a slot remembers the
// tile its unit stands on, so a slot dies when that unit is killed during our turn
struct ThreatMap {
    int attacker_side = 0;
    int slot_count = 0;
    std::uint16_t slot_tile[kMaxUnits];
    std::uint64_t attackers[kMaxTiles]; // bit per slot
};

void BuildThreats(const Map& map, const Position& pos, int attacker_side, ThreatMap* threats);

// funds a unit standing on `tile` should expect to lose next turn: the best hit on it plus
// half the second best, at most the unit's worth
int Exposure(const Map& map, const Position& pos, const ThreatMap& threats, const Unit& unit,
             int tile);

} // namespace shoal
