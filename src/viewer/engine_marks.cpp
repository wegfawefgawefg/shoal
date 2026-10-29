// the engine's best turn on the board: each unit's walk as an arrow, numbered in the order
// the engine plays them, with strikes, captures and builds labelled
#include "analysis_view.hpp"
#include "board_draw.hpp"
#include "palette.hpp"
#include "replay.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace shoal {

namespace {

bool Walks(const Move& move) {
    return move.kind != MoveKind::EndTurn && move.kind != MoveKind::Build && move.from != move.to;
}

void DrawStrike(const Canvas& canvas, const Move& move) {
    Vec2 from = TileCentre(canvas, move.to);
    Vec2 to = TileCentre(canvas, move.target);
    float t = canvas.tile;
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float len = std::max(1e-3f, std::sqrt(dx * dx + dy * dy));
    Vec2 a{from.x + dx / len * t * 0.3f, from.y + dy / len * t * 0.3f};
    Vec2 b{to.x - dx / len * t * 0.36f, to.y - dy / len * t * 0.36f};
    RoundLine(canvas.renderer, a, b, t * 0.1f, kInk);
    RoundLine(canvas.renderer, a, b, t * 0.055f, kBest);
    FillArc(canvas.renderer, to, t * 0.3f, t * 0.39f, 0, 6.2832f, kInk);
    FillArc(canvas.renderer, to, t * 0.32f, t * 0.37f, 0, 6.2832f, kBest);
}

std::string Label(const Move& move, int number) {
    std::string text = std::to_string(number);
    if (move.kind == MoveKind::Capture) {
        text += " cap";
    } else if (move.kind == MoveKind::Build) {
        text += " " + std::string(Spec(move.build).short_name);
    }
    return text;
}

} // namespace

void DrawBestTurnUnder(const Canvas& canvas, const State& state) {
    const std::vector<Move>* best = ShownBestTurn(state);
    if (best == nullptr) {
        return;
    }
    const Map& map = *canvas.map;
    Position pos = state.positions[static_cast<std::size_t>(state.ply)];
    for (const Move& move : *best) {
        if (Walks(move)) {
            DrawPathArrow(canvas, MovePath(map, pos, move), WithAlpha(kBest, 235));
        }
        ApplyMove(map, &pos, move);
    }
}

void DrawBestTurnOver(const Canvas& canvas, const State& state) {
    const std::vector<Move>* best = ShownBestTurn(state);
    if (best == nullptr) {
        return;
    }
    for (const Move& move : *best) {
        if (move.kind == MoveKind::Attack) {
            DrawStrike(canvas, move);
        }
    }
    // numbers last so nothing covers them
    int number = 1;
    for (const Move& move : *best) {
        Frame f = TileFrame(canvas, move.to);
        Vec2 at = move.kind == MoveKind::Build ? f.P(0.5f, 0.84f) : f.P(0.24f, 0.2f);
        DrawChip(canvas, at, Label(move, number), kBest, kInk);
        number++;
    }
}

} // namespace shoal
