#pragma once

#include "draw.hpp"
#include "game/position.hpp"
#include "io/map_file.hpp"
#include "text.hpp"

#include <SDL3/SDL.h>
#include <cstdint>
#include <memory>
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
    Rect eval_bar;  // left of the board
    Rect panel;     // the side panel
    Rect header;    // day, players, funds
    Rect report;    // the line about the last move
    Rect engine;    // the engine's verdict and best turn for the shown position
    Rect graph;     // the eval graph; zero size when there is nothing to plot
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

// a planner search running on its own thread (live_analysis.cpp)
struct LiveJob;

// the result of the last live analysis (key A); shown only on the position it was made for
struct LiveResult {
    bool valid = false;
    int ply = 0;
    std::uint64_t hash = 0;
    int side = 0;  // side to move there
    int score = 0; // from that side's view
    std::vector<Move> best;
    int turns = 0;
    double seconds = 0;
};

struct LiveAnalysis {
    std::vector<std::shared_ptr<LiveJob>> jobs; // running or waiting to be joined
    int latest_id = 0;                          // only this job's result is kept
    LiveResult result;
    double seconds = 1.5; // per press: two searches of half this
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
    LiveAnalysis live;
    bool show_best = true; // the engine's best turn as arrows on the board (B)

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
