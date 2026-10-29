#include "board_draw.hpp"
#include "palette.hpp"
#include "unit_art.hpp"

#include <cmath>
#include <string>

namespace shoal {

namespace {

constexpr float kPi = 3.14159265f;

Vec2 Direction(Vec2 a, Vec2 b) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-4f) {
        return Vec2{1, 0};
    }
    return Vec2{dx / len, dy / len};
}

void TintTile(const Canvas& canvas, int tile, Color color) {
    Frame f = TileFrame(canvas, tile);
    FillRect(canvas.renderer, f.R(0, 0, 1, 1), color);
}

} // namespace

void DrawPathArrow(const Canvas& canvas, const std::vector<int>& path, Color fill) {
    if (path.size() < 2) {
        return;
    }
    float t = canvas.tile;
    std::vector<Vec2> points;
    for (int tile : path) {
        points.push_back(TileCentre(canvas, tile));
    }
    Vec2 last = points[points.size() - 2];
    Vec2 end = points.back();
    Vec2 dir = Direction(last, end);
    // the head ends where the unit's body starts, so it stays visible under the unit
    Vec2 tip{end.x - dir.x * t * 0.3f, end.y - dir.y * t * 0.3f};
    Vec2 head_base{tip.x - dir.x * t * 0.26f, tip.y - dir.y * t * 0.26f};
    points.back() = head_base;
    float width = t * 0.15f;
    float head = t * 0.22f;
    Vec2 side{-dir.y, dir.x};
    Vec2 tri[3] = {tip,
                   {head_base.x + side.x * head, head_base.y + side.y * head},
                   {head_base.x - side.x * head, head_base.y - side.y * head}};
    // ink first, then the fill on top
    for (int pass = 0; pass < 2; pass++) {
        float grow = pass == 0 ? t * 0.035f : 0;
        Color color = pass == 0 ? WithAlpha(kInk, 200) : fill;
        for (std::size_t i = 0; i + 1 < points.size(); i++) {
            RoundLine(canvas.renderer, points[i], points[i + 1], width + grow * 2, color);
        }
        if (pass == 0) {
            Vec2 grown[3];
            InflatePoly(tri, 3, grow, grown);
            FillPoly(canvas.renderer, grown, 3, color);
        } else {
            FillPoly(canvas.renderer, tri, 3, color);
        }
    }
}

namespace {

void DrawReticle(const Canvas& canvas, int tile) {
    Vec2 c = TileCentre(canvas, tile);
    float t = canvas.tile;
    FillArc(canvas.renderer, c, t * 0.29f, t * 0.38f, 0, 2 * kPi, kInk);
    FillArc(canvas.renderer, c, t * 0.31f, t * 0.36f, 0, 2 * kPi, kAttack);
    for (int i = 0; i < 4; i++) {
        float a = static_cast<float>(i) * kPi * 0.5f;
        Vec2 d{std::cos(a), std::sin(a)};
        Vec2 p0{c.x + d.x * t * 0.24f, c.y + d.y * t * 0.24f};
        Vec2 p1{c.x + d.x * t * 0.44f, c.y + d.y * t * 0.44f};
        Line(canvas.renderer, p0, p1, t * 0.075f, kInk);
        Line(canvas.renderer, p0, p1, t * 0.04f, kAttack);
    }
}

// a destroyed unit stays as a grey ghost under a red cross
void DrawWreck(const Canvas& canvas, int tile, UnitType type, int owner) {
    Frame f = TileFrame(canvas, tile);
    DrawUnit(canvas.renderer, type, owner, f.x, f.y, f.s, true);
    float t = canvas.tile;
    Vec2 a = f.P(0.24f, 0.24f);
    Vec2 b = f.P(0.76f, 0.76f);
    Vec2 c = f.P(0.76f, 0.24f);
    Vec2 d = f.P(0.24f, 0.76f);
    RoundLine(canvas.renderer, a, b, t * 0.13f, kInk);
    RoundLine(canvas.renderer, c, d, t * 0.13f, kInk);
    RoundLine(canvas.renderer, a, b, t * 0.08f, kAttack);
    RoundLine(canvas.renderer, c, d, t * 0.08f, kAttack);
}

std::string Loss(int before, int after) {
    return "-" + std::to_string(before - after);
}

void DrawAttack(const Canvas& canvas, const MoveReport& report, const Position& before) {
    const Move& move = report.move;
    int target_owner = before.units[before.unit_at[move.target]].owner;
    // a strike line from the attacker to the target
    Vec2 from = TileCentre(canvas, move.to);
    Vec2 to = TileCentre(canvas, move.target);
    Vec2 dir = Direction(from, to);
    float t = canvas.tile;
    Vec2 a{from.x + dir.x * t * 0.3f, from.y + dir.y * t * 0.3f};
    Vec2 b{to.x - dir.x * t * 0.42f, to.y - dir.y * t * 0.42f};
    RoundLine(canvas.renderer, a, b, t * 0.09f, kInk);
    RoundLine(canvas.renderer, a, b, t * 0.045f, kAttack);

    if (report.target_hp_after == 0) {
        DrawWreck(canvas, move.target, report.target, target_owner);
    } else {
        DrawReticle(canvas, move.target);
    }
    if (report.mover_hp_after == 0) {
        DrawWreck(canvas, move.to, report.mover, report.side);
    }
    Frame tf = TileFrame(canvas, move.target);
    std::string hit = report.target_hp_after == 0
                          ? "destroyed"
                          : Loss(report.target_hp_before, report.target_hp_after);
    DrawChip(canvas, tf.P(0.78f, 0.1f), hit, kAttack, kText);
    if (report.mover_hp_after < report.mover_hp_before) {
        Frame mf = TileFrame(canvas, move.to);
        std::string back = report.mover_hp_after == 0
                               ? "lost"
                               : Loss(report.mover_hp_before, report.mover_hp_after);
        DrawChip(canvas, mf.P(0.78f, 0.1f), back, Color{0x6a, 0x2a, 0x30}, kText);
    }
}

void DrawCapture(const Canvas& canvas, const MoveReport& report) {
    Frame f = TileFrame(canvas, report.move.to);
    if (report.capture_left == 0) {
        DrawChip(canvas, f.P(0.5f, 0.12f), "captured", kPlayer[report.side], kInk);
        return;
    }
    std::string text =
        std::to_string(kCapturePoints - report.capture_left) + "/" + std::to_string(kCapturePoints);
    DrawChip(canvas, f.P(0.5f, 0.22f), text, kPanelRaised, kText);
}

} // namespace

void DrawMoveTint(const Canvas& canvas, const MoveReport& report) {
    if (!report.valid || report.move.kind == MoveKind::EndTurn) {
        return;
    }
    TintTile(canvas, report.move.from, kMoveTint);
    if (report.move.to != report.move.from) {
        TintTile(canvas, report.move.to, kMoveTint);
    }
}

void DrawMovePath(const Canvas& canvas, const MoveReport& report) {
    if (report.valid) {
        DrawPathArrow(canvas, report.path, kArrow);
    }
}

void DrawMoveMarks(const Canvas& canvas, const MoveReport& report, const Position& before) {
    if (!report.valid) {
        return;
    }
    switch (report.move.kind) {
    case MoveKind::EndTurn:
        return;
    case MoveKind::Build: {
        Frame f = TileFrame(canvas, report.move.to);
        DrawChip(canvas, f.P(0.5f, 0.12f), "built", kPlayer[report.side], kInk);
        return;
    }
    case MoveKind::Wait:
        return;
    case MoveKind::Join: {
        Frame f = TileFrame(canvas, report.move.to);
        DrawChip(canvas, f.P(0.5f, 0.12f), "joined", kPlayer[report.side], kInk);
        return;
    }
    case MoveKind::Load: {
        Frame f = TileFrame(canvas, report.move.to);
        DrawChip(canvas, f.P(0.5f, 0.12f), "loaded", kPlayer[report.side], kInk);
        return;
    }
    case MoveKind::Unload: {
        Frame f = TileFrame(canvas, report.move.target);
        DrawChip(canvas, f.P(0.5f, 0.12f), "dropped", kPlayer[report.side], kInk);
        return;
    }
    case MoveKind::Attack:
        DrawAttack(canvas, report, before);
        return;
    case MoveKind::Capture:
        DrawCapture(canvas, report);
        return;
    }
}

} // namespace shoal
