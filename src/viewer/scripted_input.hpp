#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <vector>

namespace shoal {

// turns "right,right,up,wheel:-3,click:640:400" into events for headless tests of the controls;
// keys: left right up down home end p e esc; mouse: move:x:y click:x:y rclick:x:y wheel:n:x:y
bool ParseInputScript(const std::string& script, std::vector<SDL_Event>* events,
                      std::string* error);

} // namespace shoal
