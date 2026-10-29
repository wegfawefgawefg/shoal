#include "input.hpp"

#include "analysis_panel.hpp"
#include "layout.hpp"
#include "engine_opponent.hpp"
#include "live_analysis.hpp"
#include "play_mode.hpp"
#include "replay.hpp"
#include "side_panel.hpp"

namespace shoal {

namespace {

// drops moves entered by hand and goes back to the file as it is on disk
void ReloadRecord(State* state) {
    int ply = state->ply;
    std::string error;
    if (!LoadRecord(state, state->path, &error)) {
        SDL_Log("reload %s: %s", state->path.c_str(), error.c_str());
        return;
    }
    state->edited_from = -1;
    SetPly(state, ply);
    state->scroll_to_ply = true;
}

void StepTo(State* state, int ply) {
    SetPly(state, ply);
    state->scroll_to_ply = true;
}

void HandleKey(State* state, SDL_Keycode key) {
    switch (key) {
    case SDLK_LEFT:
        StepTo(state, state->ply - 1);
        return;
    case SDLK_RIGHT:
        StepTo(state, state->ply + 1);
        return;
    case SDLK_UP:
        StepTo(state, PrevTurnPly(*state, state->ply));
        return;
    case SDLK_DOWN:
        StepTo(state, NextTurnPly(*state, state->ply));
        return;
    case SDLK_HOME:
        StepTo(state, 0);
        return;
    case SDLK_END:
        StepTo(state, LastPly(*state));
        return;
    case SDLK_P:
        state->mode = state->mode == Mode::Play ? Mode::Replay : Mode::Play;
        state->play = PlayState{};
        return;
    case SDLK_E:
        if (state->mode == Mode::Play) {
            PlayEndTurn(state);
        }
        return;
    case SDLK_ESCAPE:
        PlayCancel(state);
        return;
    case SDLK_R:
        ReloadRecord(state);
        return;
    case SDLK_A:
        StartLiveAnalysis(state);
        return;
    case SDLK_B:
        state->show_best = !state->show_best;
        return;
    case SDLK_O:
        ToggleEngineOpponent(state);
        return;
    case SDLK_Q:
        state->running = false;
        return;
    default:
        return;
    }
}

void HandleClick(State* state, float x, float y, Uint8 button) {
    if (button == SDL_BUTTON_RIGHT) {
        PlayCancel(state);
        return;
    }
    if (button != SDL_BUTTON_LEFT) {
        return;
    }
    int row = MoveRowAt(*state, x, y);
    if (row >= 0) {
        SetPly(state, RowPly(*state, row));
        return;
    }
    int graph_ply = GraphPlyAt(*state, x, y);
    if (graph_ply >= 0) {
        StepTo(state, graph_ply);
        return;
    }
    if (state->mode == Mode::Play) {
        PlayClick(state, x, y);
    }
}

} // namespace

void HandleEvent(State* state, SDL_Event* event) {
    // scripted events in screenshot mode are already in render pixels
    if (state->window) {
        SDL_ConvertEventToRenderCoordinates(state->renderer, event);
    }
    switch (event->type) {
    case SDL_EVENT_QUIT:
        state->running = false;
        return;
    case SDL_EVENT_KEY_DOWN:
        HandleKey(state, event->key.key);
        return;
    case SDL_EVENT_MOUSE_MOTION:
        state->mouse_x = event->motion.x;
        state->mouse_y = event->motion.y;
        state->hover_tile = TileAtPoint(*state, state->mouse_x, state->mouse_y);
        state->hover_row = MoveRowAt(*state, state->mouse_x, state->mouse_y);
        return;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        HandleClick(state, event->button.x, event->button.y, event->button.button);
        return;
    case SDL_EVENT_MOUSE_WHEEL:
        if (Contains(state->layout.move_list, event->wheel.mouse_x, event->wheel.mouse_y)) {
            ScrollMoveList(state, -event->wheel.y * Ui(*state, 23) * 3);
        }
        return;
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        state->hover_tile = -1;
        state->hover_row = -1;
        return;
    default:
        return;
    }
}

} // namespace shoal
