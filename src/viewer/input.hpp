#pragma once

#include "state.hpp"

namespace shoal {

// keys step through the game, the mouse picks rows, scrolls the list and plays moves
void HandleEvent(State* state, SDL_Event* event);

} // namespace shoal
