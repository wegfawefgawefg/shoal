#include "play_mode.hpp"

#include "layout.hpp"
#include "palette.hpp"
#include "replay.hpp"

#include <memory>
#include <string>

namespace shoal {

namespace {

const Position& Current(const State& state) {
    return state.positions[static_cast<std::size_t>(state.ply)];
}

std::vector<Move> LegalMoves(const State& state) {
    auto list = std::make_unique<MoveList>();
    GenerateMoves(state.game.map, Current(state), list.get());
    return std::vector<Move>(list->moves, list->moves + list->count);
}

// attacks first, then capture, then wait, so the likely action sits on top
int ActionRank(const Move& move) {
    switch (move.kind) {
    case MoveKind::Attack:
        return 0;
    case MoveKind::Capture:
        return 1;
    case MoveKind::Unload:
        return 1;
    case MoveKind::Wait:
    case MoveKind::Join:
    case MoveKind::Load:
        return 2;
    default:
        return 3;
    }
}

void OpenMenuAt(State* state, int dest) {
    PlayState& play = state->play;
    play.dest = dest;
    play.menu.clear();
    for (int rank = 0; rank < 4; rank++) {
        for (const Move& move : play.unit_moves) {
            if (move.to == dest && ActionRank(move) == rank) {
                play.menu.push_back(move);
            }
        }
    }
}

std::string ActionLabel(const State& state, const Move& move) {
    const Map& map = state.game.map;
    const Position& pos = Current(state);
    switch (move.kind) {
    case MoveKind::Wait:
        return "Wait";
    case MoveKind::Join:
        return "Join";
    case MoveKind::Load:
        return "Load";
    case MoveKind::Unload: {
        const Unit& transport = pos.units[pos.unit_at[move.from]];
        return "Drop " + std::string(Spec(transport.cargo[move.slot].type).name) + " on " +
               TileName(map, move.target);
    }
    case MoveKind::Capture: {
        int prop = map.property_at[move.to];
        int hp = DisplayHp(pos.units[pos.unit_at[move.from]].hp);
        int left = pos.capture[prop] - hp;
        return left <= 0 ? "Capture (takes it)" : "Capture (" + std::to_string(left) + " left)";
    }
    case MoveKind::Attack: {
        // preview by playing it on a copy: first strike and counter, exactly as the engine does
        Position after = pos;
        ApplyMove(map, &after, move);
        const Unit& defender = pos.units[pos.unit_at[move.target]];
        int hp_before = DisplayHp(defender.hp);
        int hp_after = after.unit_at[move.target] == kNoUnit
                           ? 0
                           : DisplayHp(after.units[after.unit_at[move.target]].hp);
        int mine_before = DisplayHp(pos.units[pos.unit_at[move.from]].hp);
        int mine_after = after.unit_at[move.to] == kNoUnit
                             ? 0
                             : DisplayHp(after.units[after.unit_at[move.to]].hp);
        std::string label = "Fire at " + TileName(map, move.target) + " " +
                            std::string(Spec(defender.type).name) + "  " +
                            std::to_string(hp_before) + " > " + std::to_string(hp_after);
        if (mine_after == 0) {
            label += ", lost to the counter";
        } else if (mine_after < mine_before) {
            label += ", takes " + std::to_string(mine_before - mine_after);
        }
        return label;
    }
    case MoveKind::Build:
        return "Build " + std::string(Spec(move.build).name) + "  " +
               std::to_string(Spec(move.build).cost);
    case MoveKind::EndTurn:
        return "End turn";
    }
    return "?";
}

} // namespace

bool SelectTile(State* state, int tile) {
    const Map& map = state->game.map;
    const Position& pos = Current(*state);
    state->play = PlayState{};
    if (tile < 0 || pos.result != Result::Ongoing) {
        return false;
    }
    std::uint8_t index = pos.unit_at[tile];
    if (index != kNoUnit) {
        const Unit& unit = pos.units[index];
        if (unit.owner != pos.side || unit.done) {
            return false;
        }
        for (const Move& move : LegalMoves(*state)) {
            bool unit_move = move.kind != MoveKind::Build && move.kind != MoveKind::EndTurn;
            if (unit_move && move.from == tile) {
                state->play.unit_moves.push_back(move);
            }
        }
        state->play.selected = tile;
        return !state->play.unit_moves.empty();
    }
    // an empty own base offers what it can build
    int prop = map.property_at[tile];
    if (prop == kNoProperty || map.tiles[tile] != Terrain::Base ||
        pos.prop_owner[prop] != pos.side) {
        return false;
    }
    for (const Move& move : LegalMoves(*state)) {
        if (move.kind == MoveKind::Build && move.to == tile) {
            state->play.menu.push_back(move);
        }
    }
    if (state->play.menu.empty()) {
        return false;
    }
    state->play.dest = tile;
    return true;
}

void PlayCancel(State* state) {
    if (state->play.dest >= 0 && state->play.selected >= 0) {
        // back out of the menu, keep the unit picked
        state->play.dest = -1;
        state->play.menu.clear();
        state->play.menu_items.clear();
        return;
    }
    state->play = PlayState{};
}

void PlayEndTurn(State* state) {
    if (Current(*state).result != Result::Ongoing) {
        return;
    }
    PlayMove(state, Move{});
}

bool PlayClick(State* state, float x, float y) {
    PlayState& play = state->play;
    // a menu entry plays its move
    if (!play.menu.empty()) {
        for (std::size_t i = 0; i < play.menu_items.size() && i < play.menu.size(); i++) {
            if (Contains(play.menu_items[i], x, y)) {
                Move move = play.menu[i];
                PlayMove(state, move);
                return true;
            }
        }
    }
    int tile = TileAtPoint(*state, x, y);
    if (tile < 0) {
        PlayCancel(state);
        return false;
    }
    if (play.selected >= 0 && play.dest < 0) {
        for (const Move& move : play.unit_moves) {
            if (move.to == tile) {
                OpenMenuAt(state, tile);
                // a lone wait needs no menu
                if (play.menu.size() == 1) {
                    Move only = play.menu[0];
                    PlayMove(state, only);
                }
                return true;
            }
        }
    }
    SelectTile(state, tile);
    return true;
}

void DrawPlayUnder(const Canvas& canvas, const State& state) {
    const PlayState& play = state.play;
    if (state.mode != Mode::Play || play.selected < 0) {
        return;
    }
    int tiles = canvas.map->width * canvas.map->height;
    std::vector<std::uint8_t> reach(static_cast<std::size_t>(tiles), 0);
    for (const Move& move : play.unit_moves) {
        reach[move.to] |= 1;
        if (move.kind == MoveKind::Attack) {
            reach[move.target] |= 2;
        }
    }
    for (int tile = 0; tile < tiles; tile++) {
        Frame f = TileFrame(canvas, tile);
        if (reach[static_cast<std::size_t>(tile)] & 1) {
            FillRect(canvas.renderer, f.R(0, 0, 1, 1), kReach);
            StrokeRect(canvas.renderer, f.R(0.03f, 0.03f, 0.94f, 0.94f), f.s * 0.025f, kReachEdge);
        }
        if (reach[static_cast<std::size_t>(tile)] & 2) {
            FillRect(canvas.renderer, f.R(0, 0, 1, 1), kThreat);
        }
    }
}

void DrawPlayOver(const Canvas& canvas, const State& state) {
    const PlayState& play = state.play;
    if (state.mode != Mode::Play) {
        return;
    }
    if (play.selected >= 0) {
        Frame f = TileFrame(canvas, play.selected);
        StrokeRect(canvas.renderer, f.R(0, 0, 1, 1), f.s * 0.06f, kHighlight);
    }
    if (play.dest >= 0) {
        Frame f = TileFrame(canvas, play.dest);
        StrokeRect(canvas.renderer, f.R(0.04f, 0.04f, 0.92f, 0.92f), f.s * 0.05f, kText);
    }
}

void DrawPlayMenu(State* state) {
    PlayState& play = state->play;
    play.menu_items.clear();
    if (state->mode != Mode::Play || play.menu.empty() || play.dest < 0) {
        return;
    }
    float size = Ui(*state, 15);
    float line = LineHeight(size) + Ui(*state, 8);
    float pad = Ui(*state, 10);
    float width = 0;
    std::vector<std::string> labels;
    for (const Move& move : play.menu) {
        labels.push_back(ActionLabel(*state, move));
        float w = TextWidth(&state->font, size, labels.back());
        width = w > width ? w : width;
    }
    width += pad * 2;
    float height = line * static_cast<float>(labels.size()) + pad;
    Rect tile = TileRect(*state, play.dest);
    float x = tile.x + tile.w + Ui(*state, 6);
    float y = tile.y;
    if (x + width > state->layout.panel.x - Ui(*state, 4)) {
        x = tile.x - width - Ui(*state, 6);
    }
    if (y + height > state->layout.height - Ui(*state, 4)) {
        y = state->layout.height - height - Ui(*state, 4);
    }
    Rect box{x, y, width, height};
    FillRoundRect(state->renderer, Rect{x - 1, y - 1, width + 2, height + 2}, Ui(*state, 7),
                  kPanelLine);
    FillRoundRect(state->renderer, box, Ui(*state, 6), kPanelRaised);
    for (std::size_t i = 0; i < labels.size(); i++) {
        Rect item{x + pad * 0.4f, y + pad * 0.5f + line * static_cast<float>(i), width - pad * 0.8f,
                  line};
        play.menu_items.push_back(item);
        bool hover = Contains(item, state->mouse_x, state->mouse_y);
        if (hover) {
            FillRoundRect(state->renderer, item, Ui(*state, 4), kPanelLine);
        }
        Color color = play.menu[i].kind == MoveKind::Attack ? Color{0xff, 0x9a, 0x9f} : kText;
        DrawText(&state->font, item.x + pad * 0.6f, item.y + (line - LineHeight(size)) * 0.5f, size,
                 color, labels[i]);
    }
}

} // namespace shoal
