#include "board_draw.hpp"

#include "layout.hpp"
#include "palette.hpp"
#include "play_mode.hpp"
#include "terrain_art.hpp"
#include "unit_art.hpp"

#include <cmath>
#include <string>

namespace shoal {

namespace {

constexpr int kMaxBoardTexture = 4096;

// the board is drawn at twice its size and filtered down, which smooths every edge
float SupersampleFor(const Layout& layout) {
    float largest = layout.board.w > layout.board.h ? layout.board.w : layout.board.h;
    return largest * 2 <= kMaxBoardTexture ? 2.0f : 1.0f;
}

bool EnsureBoardTexture(State* state, int w, int h) {
    if (state->board_texture && state->board_texture_w == w && state->board_texture_h == h) {
        return true;
    }
    if (state->board_texture) {
        SDL_DestroyTexture(state->board_texture);
    }
    state->board_texture = SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_TARGET, w, h);
    if (!state->board_texture) {
        return false;
    }
    SDL_SetTextureScaleMode(state->board_texture, SDL_SCALEMODE_LINEAR);
    state->board_texture_w = w;
    state->board_texture_h = h;
    return true;
}

void DrawGrid(const Canvas& canvas) {
    const Map& map = *canvas.map;
    float w = canvas.tile * static_cast<float>(map.width);
    float h = canvas.tile * static_cast<float>(map.height);
    float line = std::fmax(1.0f, canvas.tile * 0.018f);
    Color color = WithAlpha(kInk, 46);
    for (int x = 1; x < map.width; x++) {
        float px = canvas.tile * static_cast<float>(x) - line * 0.5f;
        FillRect(canvas.renderer, Rect{px, 0, line, h}, color);
    }
    for (int y = 1; y < map.height; y++) {
        float py = canvas.tile * static_cast<float>(y) - line * 0.5f;
        FillRect(canvas.renderer, Rect{0, py, w, line}, color);
    }
}

void DrawCaptureBars(const Canvas& canvas, const Position& pos) {
    const Map& map = *canvas.map;
    for (int prop = 0; prop < map.property_count; prop++) {
        int left = pos.capture[prop];
        int tile = map.property_tiles[prop];
        if (left >= kCapturePoints || pos.unit_at[tile] == kNoUnit) {
            continue;
        }
        int capturer = pos.units[pos.unit_at[tile]].owner;
        Frame f = TileFrame(canvas, tile);
        float done = static_cast<float>(kCapturePoints - left) / kCapturePoints;
        FillRoundRect(canvas.renderer, f.R(0.1f, 0.04f, 0.8f, 0.09f), 0.04f * f.s, kInk);
        FillRoundRect(canvas.renderer, f.R(0.12f, 0.055f, 0.76f * done, 0.06f), 0.03f * f.s,
                      kPlayer[capturer]);
    }
}

void DrawHpBadge(const Canvas& canvas, const Frame& f, int hp) {
    Rect badge = f.R(0.64f, 0.6f, 0.32f, 0.36f);
    FillRoundRect(canvas.renderer, badge, 0.07f * f.s, WithAlpha(kInk, 235));
    std::string text = std::to_string(hp);
    float size = 0.3f * f.s;
    float y = badge.y + (badge.h - LineHeight(size)) * 0.5f;
    DrawTextAligned(canvas.font, badge.x + badge.w * 0.5f, y, size, kText, text, Align::Centre);
}

// a transport's riders: one small pip each in the top left, in the owner's colour
void DrawCargoPips(const Canvas& canvas, const Frame& f, const Unit& unit) {
    for (int i = 0; i < unit.cargo_count; i++) {
        Rect pip = f.R(0.06f + 0.17f * static_cast<float>(i), 0.06f, 0.14f, 0.14f);
        FillRoundRect(canvas.renderer, pip, 0.07f * f.s, WithAlpha(kInk, 235));
        Rect dot = f.R(0.09f + 0.17f * static_cast<float>(i), 0.09f, 0.08f, 0.08f);
        FillRoundRect(canvas.renderer, dot, 0.04f * f.s, kPlayer[unit.owner]);
    }
}

// an amber drop in the top right corner: a quarter tank or less
void DrawFuelWarning(const Canvas& canvas, const Frame& f) {
    Color amber{0xf4, 0xb7, 0x3c};
    FillCircle(canvas.renderer, f.P(0.85f, 0.15f), 0.11f * f.s, WithAlpha(kInk, 235));
    FillCircle(canvas.renderer, f.P(0.85f, 0.18f), 0.06f * f.s, amber);
    FillTriangle(canvas.renderer, f.P(0.85f, 0.06f), f.P(0.795f, 0.165f), f.P(0.905f, 0.165f),
                 amber);
}

void DrawUnits(const Canvas& canvas, const Position& pos) {
    // top rows first so lower units overlap the ones behind them
    const Map& map = *canvas.map;
    for (int tile = 0; tile < map.width * map.height; tile++) {
        std::uint8_t index = pos.unit_at[tile];
        if (index == kNoUnit) {
            continue;
        }
        const Unit& unit = pos.units[index];
        Frame f = TileFrame(canvas, tile);
        bool dimmed = unit.done != 0 && unit.owner == pos.side;
        DrawUnit(canvas.renderer, unit.type, unit.owner, f.x, f.y, f.s, dimmed, unit.dived != 0);
        int hp = DisplayHp(unit.hp);
        if (hp < 10) {
            DrawHpBadge(canvas, f, hp);
        }
        DrawCargoPips(canvas, f, unit);
        if (LowFuel(unit)) {
            DrawFuelWarning(canvas, f);
        }
    }
}

void DrawCoordinates(State* state) {
    const Layout& l = state->layout;
    const Map& map = state->game.map;
    if (l.tile < 14) {
        return;
    }
    float size = Ui(*state, 12);
    float line = LineHeight(size);
    for (int x = 0; x < map.width; x++) {
        std::string name = TileName(map, TileAt(map, x, 0));
        name.pop_back();
        float cx = l.board.x + (static_cast<float>(x) + 0.5f) * l.tile;
        DrawTextAligned(&state->font, cx, l.board.y - line - Ui(*state, 3), size, kTextFaint, name,
                        Align::Centre);
    }
    for (int y = 0; y < map.height; y++) {
        std::string name = std::to_string(y + 1);
        float cy = l.board.y + (static_cast<float>(y) + 0.5f) * l.tile - line * 0.5f;
        DrawTextAligned(&state->font, l.board.x - Ui(*state, 6), cy, size, kTextFaint, name,
                        Align::Right);
    }
}

} // namespace

Frame TileFrame(const Canvas& canvas, int tile) {
    return Frame{static_cast<float>(TileX(*canvas.map, tile)) * canvas.tile,
                 static_cast<float>(TileY(*canvas.map, tile)) * canvas.tile, canvas.tile};
}

Vec2 TileCentre(const Canvas& canvas, int tile) {
    Frame f = TileFrame(canvas, tile);
    return f.P(0.5f, 0.5f);
}

void DrawChip(const Canvas& canvas, Vec2 centre, std::string_view text, Color fill, Color ink) {
    float size = canvas.tile * 0.24f;
    float width = TextWidth(canvas.font, size, text) + canvas.tile * 0.16f;
    float height = canvas.tile * 0.3f;
    Rect outer{centre.x - width * 0.5f - 2, centre.y - height * 0.5f - 2, width + 4, height + 4};
    FillRoundRect(canvas.renderer, outer, height * 0.5f + 2, kInk);
    Rect chip{centre.x - width * 0.5f, centre.y - height * 0.5f, width, height};
    FillRoundRect(canvas.renderer, chip, height * 0.5f, fill);
    float y = chip.y + (chip.h - LineHeight(size)) * 0.5f;
    DrawTextAligned(canvas.font, centre.x, y, size, ink, text, Align::Centre);
}

void DrawBoard(State* state) {
    const Layout& l = state->layout;
    const Map& map = state->game.map;
    const Position& pos = state->positions[static_cast<std::size_t>(state->ply)];
    float scale = SupersampleFor(l);
    int tw = static_cast<int>(l.board.w * scale);
    int th = static_cast<int>(l.board.h * scale);
    if (tw <= 0 || th <= 0 || !EnsureBoardTexture(state, tw, th)) {
        return;
    }

    // draw into the board texture
    SDL_SetRenderTarget(state->renderer, state->board_texture);
    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
    SDL_RenderClear(state->renderer);
    Canvas canvas{state->renderer, &state->font, &map, l.tile * scale};
    for (int tile = 0; tile < map.width * map.height; tile++) {
        Frame f = TileFrame(canvas, tile);
        DrawTerrain(state->renderer, map, tile, f.x, f.y, f.s);
    }
    DrawGrid(canvas);
    DrawMoveTint(canvas, state->report);
    for (int prop = 0; prop < map.property_count; prop++) {
        int tile = map.property_tiles[prop];
        Frame f = TileFrame(canvas, tile);
        DrawProperty(state->renderer, map.tiles[tile], pos.prop_owner[prop], f.x, f.y, f.s);
    }
    DrawCaptureBars(canvas, pos);
    DrawPlayUnder(canvas, *state);
    DrawMovePath(canvas, state->report);
    DrawBestTurnUnder(canvas, *state);
    DrawUnits(canvas, pos);
    if (state->ply > 0) {
        const Position& before = state->positions[static_cast<std::size_t>(state->ply - 1)];
        DrawMoveMarks(canvas, state->report, before);
    }
    DrawBestTurnOver(canvas, *state);
    DrawPlayOver(canvas, *state);
    SDL_SetRenderTarget(state->renderer, nullptr);

    // put it on screen
    SDL_FRect dst{l.board.x, l.board.y, l.board.w, l.board.h};
    SDL_RenderTexture(state->renderer, state->board_texture, nullptr, &dst);
    if (state->hover_tile >= 0) {
        StrokeRect(state->renderer, TileRect(*state, state->hover_tile), Ui(*state, 2),
                   WithAlpha(kText, 150));
    }
    DrawCoordinates(state);
}

} // namespace shoal
