#pragma once

#include "draw.hpp"
#include "game/position.hpp"
#include "io/map_file.hpp"
#include "text.hpp"

#include <SDL3/SDL.h>
#include <string>
#include <vector>

namespace shoal {

// where everything sits this frame, in render pixels
struct Layout {
    float width = 0;
    float height = 0;
    float ui = 1;   // text and padding scale, 1 at about 800 pixels tall
    Rect board;     // the tiles
    float tile = 0; // tile size
    Rect eval_bar;  // zero size when the game has no evals
    Rect panel;     // the side panel
    Rect header;    // day, players, funds
    Rect report;    // the line about the last move
    Rect move_list; // the scrolling move list
    Rect footer;    // key hints
};

// a row in the move list: a turn heading or one move
struct MoveRow {
    bool heading = false;
    int day = 1;
    int side = 0;
    int move_index = -1; // index into game.moves; the row shows the position after it
};

// what the move into the current ply did, for the board marks and the panel line
struct MoveReport {
    bool valid = false;
    Move move;
    int side = 0;
    UnitType mover = UnitType::Infantry;
    std::vector<int> path; // tiles walked, start to destination
    bool attack = false;
    UnitType target = UnitType::Infantry;
    int target_hp_before = 0; // board hit points; after is 0 when destroyed
    int target_hp_after = 0;
    int mover_hp_before = 0;
    int mover_hp_after = 0;
    bool capture = false;
    int capture_left = 0; // points left on the property; 0 once it changed hands
};

enum class Mode : std::uint8_t {
    Replay,
    Play, // the owner enters moves from the current ply on
};

// play mode: pick a unit, then a destination, then an action from the menu
struct PlayState {
    int selected = -1;            // tile of the chosen unit
    int dest = -1;                // destination waiting on the action menu
    std::vector<Move> unit_moves; // every legal move of the chosen unit
    std::vector<Move> menu;       // actions offered at dest (or builds at a base)
    std::vector<Rect> menu_items; // where the menu entries were drawn, for clicks
    int hover_item = -1;
};

struct State {
    // the game
    std::string path;
    GameRecord game;
    bool is_game_file = false;
    std::vector<Position> positions; // positions[k] is the position after k moves
    int ply = 0;
    std::vector<MoveRow> rows;
    MoveReport report;
    Mode mode = Mode::Replay;
    PlayState play;
    int edited_from = -1; // first ply entered by hand, -1 when the line is the file's

    // window
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* board_texture = nullptr; // the board drawn at a higher scale, then filtered
    int board_texture_w = 0;
    int board_texture_h = 0;
    Font font;
    Layout layout;
    float list_scroll = 0;
    bool scroll_to_ply = true; // keep the current move in view after keyboard steps
    float mouse_x = -1;
    float mouse_y = -1;
    int hover_tile = -1;
    int hover_row = -1;
    bool running = true;
};

} // namespace shoal
