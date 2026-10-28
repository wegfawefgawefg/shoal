#include "scripted_input.hpp"

#include <cstdlib>

namespace shoal {

namespace {

std::vector<std::string> Split(const std::string& text, char sep) {
    std::vector<std::string> parts;
    std::string part;
    for (char c : text) {
        if (c == sep) {
            parts.push_back(part);
            part.clear();
        } else {
            part += c;
        }
    }
    parts.push_back(part);
    return parts;
}

bool KeyFor(const std::string& name, SDL_Keycode* key) {
    struct Named {
        const char* name;
        SDL_Keycode key;
    };
    const Named keys[] = {{"left", SDLK_LEFT}, {"right", SDLK_RIGHT}, {"up", SDLK_UP},
                          {"down", SDLK_DOWN}, {"home", SDLK_HOME},   {"end", SDLK_END},
                          {"p", SDLK_P},       {"e", SDLK_E},         {"esc", SDLK_ESCAPE},
                          {"r", SDLK_R}};
    for (const Named& named : keys) {
        if (name == named.name) {
            *key = named.key;
            return true;
        }
    }
    return false;
}

float Number(const std::string& text) {
    return static_cast<float>(std::atof(text.c_str()));
}

} // namespace

bool ParseInputScript(const std::string& script, std::vector<SDL_Event>* events,
                      std::string* error) {
    for (const std::string& token : Split(script, ',')) {
        std::vector<std::string> args = Split(token, ':');
        SDL_Event event;
        SDL_zero(event);
        SDL_Keycode key = 0;
        if (args.size() == 1 && KeyFor(args[0], &key)) {
            event.type = SDL_EVENT_KEY_DOWN;
            event.key.key = key;
        } else if (args.size() == 3 && args[0] == "move") {
            event.type = SDL_EVENT_MOUSE_MOTION;
            event.motion.x = Number(args[1]);
            event.motion.y = Number(args[2]);
        } else if (args.size() == 3 && (args[0] == "click" || args[0] == "rclick")) {
            event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
            event.button.button = args[0] == "click" ? SDL_BUTTON_LEFT : SDL_BUTTON_RIGHT;
            event.button.x = Number(args[1]);
            event.button.y = Number(args[2]);
        } else if (args.size() == 4 && args[0] == "wheel") {
            event.type = SDL_EVENT_MOUSE_WHEEL;
            event.wheel.y = Number(args[1]);
            event.wheel.mouse_x = Number(args[2]);
            event.wheel.mouse_y = Number(args[3]);
        } else {
            *error = "bad event '" + token + "'";
            return false;
        }
        events->push_back(event);
    }
    return true;
}

} // namespace shoal
