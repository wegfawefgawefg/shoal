#include "unit_art.hpp"

#include "palette.hpp"
#include "unit_parts.hpp"

#include <cmath>
#include <initializer_list>

namespace shoal {

namespace {

void AddSoldierHead(Art* art, float cx, float cy) {
    AddCircle(art, Tone::Body, cx, cy, 0.085f);
    AddCircle(art, Tone::Face, cx + 0.03f, cy + 0.03f, 0.052f, true);
    AddRect(art, Tone::Deep, cx - 0.085f, cy - 0.012f, 0.17f, 0.03f, true);
}

void BuildInfantry(Art* art) {
    AddCapsule(art, Tone::Deep, {0.45f, 0.6f}, {0.4f, 0.8f}, 0.075f);
    AddCapsule(art, Tone::Deep, {0.54f, 0.6f}, {0.6f, 0.8f}, 0.075f);
    AddCapsule(art, Tone::Body, {0.49f, 0.44f}, {0.49f, 0.6f}, 0.19f);
    AddSoldierHead(art, 0.5f, 0.28f);
    AddCapsule(art, Tone::Dark, {0.34f, 0.6f}, {0.8f, 0.4f}, 0.04f);
    AddCapsule(art, Tone::Light, {0.48f, 0.47f}, {0.64f, 0.5f}, 0.06f, true);
}

void BuildMech(Art* art) {
    AddRect(art, Tone::Deep, 0.3f, 0.4f, 0.1f, 0.22f);
    AddCapsule(art, Tone::Deep, {0.44f, 0.6f}, {0.36f, 0.8f}, 0.09f);
    AddCapsule(art, Tone::Deep, {0.56f, 0.6f}, {0.64f, 0.8f}, 0.09f);
    AddCapsule(art, Tone::Body, {0.5f, 0.45f}, {0.5f, 0.6f}, 0.24f);
    AddSoldierHead(art, 0.52f, 0.29f);
    // launcher tube over the shoulder
    AddCapsule(art, Tone::Metal, {0.22f, 0.42f}, {0.84f, 0.36f}, 0.09f);
    AddRect(art, Tone::Body, 0.78f, 0.31f, 0.08f, 0.1f);
    AddRect(art, Tone::Dark, 0.4f, 0.36f, 0.05f, 0.08f, true);
}

void BuildRecon(Art* art) {
    AddWheel(art, 0.27f, 0.75f, 0.09f);
    AddWheel(art, 0.72f, 0.75f, 0.09f);
    AddPoly(art, Tone::Body,
            {{0.1f, 0.58f}, {0.84f, 0.56f}, {0.92f, 0.64f}, {0.9f, 0.72f}, {0.1f, 0.72f}});
    AddPoly(art, Tone::Body, {{0.3f, 0.58f}, {0.64f, 0.58f}, {0.58f, 0.44f}, {0.36f, 0.44f}});
    AddPoly(art, Tone::Glass, {{0.5f, 0.56f}, {0.61f, 0.56f}, {0.565f, 0.47f}, {0.5f, 0.47f}},
            true);
    AddCapsule(art, Tone::Dark, {0.38f, 0.4f}, {0.62f, 0.37f}, 0.035f);
    AddRect(art, Tone::Deep, 0.1f, 0.65f, 0.8f, 0.04f, true);
}

void BuildTank(Art* art) {
    AddTreads(art, 0.12f, 0.84f, 0.74f, 0.16f, 5, 0.04f);
    AddPoly(art, Tone::Body, {{0.1f, 0.67f}, {0.88f, 0.67f}, {0.82f, 0.53f}, {0.18f, 0.53f}});
    AddCapsule(art, Tone::Deep, {0.6f, 0.46f}, {0.92f, 0.46f}, 0.06f);
    AddPoly(art, Tone::Body, {{0.3f, 0.54f}, {0.66f, 0.54f}, {0.6f, 0.38f}, {0.36f, 0.38f}});
    AddRect(art, Tone::Light, 0.37f, 0.4f, 0.22f, 0.03f, true);
    AddRect(art, Tone::Deep, 0.14f, 0.62f, 0.72f, 0.03f, true);
}

void BuildMdTank(Art* art) {
    AddTreads(art, 0.06f, 0.88f, 0.74f, 0.2f, 6, 0.05f);
    AddPoly(art, Tone::Body, {{0.04f, 0.66f}, {0.92f, 0.66f}, {0.84f, 0.48f}, {0.12f, 0.48f}});
    AddCapsule(art, Tone::Deep, {0.6f, 0.38f}, {0.9f, 0.38f}, 0.085f);
    AddRect(art, Tone::Metal, 0.88f, 0.34f, 0.08f, 0.08f);
    AddPoly(art, Tone::Body, {{0.22f, 0.5f}, {0.68f, 0.5f}, {0.64f, 0.27f}, {0.28f, 0.27f}});
    AddRect(art, Tone::Light, 0.29f, 0.29f, 0.34f, 0.035f, true);
    AddRect(art, Tone::Deep, 0.08f, 0.6f, 0.8f, 0.035f, true);
    AddRect(art, Tone::Deep, 0.3f, 0.36f, 0.3f, 0.025f, true);
}

void BuildNeotank(Art* art) {
    AddCapsule(art, Tone::Dark, {0.2f, 0.74f}, {0.76f, 0.74f}, 0.17f);
    AddCircle(art, Tone::Metal, 0.24f, 0.74f, 0.07f, true);
    AddCircle(art, Tone::Metal, 0.48f, 0.74f, 0.07f, true);
    AddCircle(art, Tone::Metal, 0.72f, 0.74f, 0.07f, true);
    AddPoly(art, Tone::Body,
            {{0.1f, 0.66f}, {0.92f, 0.66f}, {0.84f, 0.54f}, {0.36f, 0.5f}, {0.16f, 0.55f}});
    AddCapsule(art, Tone::Deep, {0.6f, 0.4f}, {0.95f, 0.4f}, 0.035f);
    AddCapsule(art, Tone::Deep, {0.6f, 0.47f}, {0.95f, 0.47f}, 0.035f);
    AddPoly(art, Tone::Body,
            {{0.32f, 0.52f}, {0.7f, 0.52f}, {0.66f, 0.38f}, {0.44f, 0.34f}, {0.36f, 0.4f}});
    AddPoly(art, Tone::Light, {{0.44f, 0.36f}, {0.62f, 0.38f}, {0.6f, 0.41f}, {0.42f, 0.4f}}, true);
}

void BuildArtillery(Art* art) {
    AddTreads(art, 0.12f, 0.72f, 0.75f, 0.15f, 4, 0.04f);
    AddCapsule(art, Tone::Deep, {0.38f, 0.52f}, {0.86f, 0.26f}, 0.07f);
    AddRect(art, Tone::Metal, 0.82f, 0.21f, 0.07f, 0.07f);
    AddRect(art, Tone::Body, 0.1f, 0.54f, 0.64f, 0.14f);
    AddCircle(art, Tone::Body, 0.38f, 0.52f, 0.1f);
    AddRect(art, Tone::Deep, 0.12f, 0.63f, 0.6f, 0.03f, true);
    AddCircle(art, Tone::Deep, 0.38f, 0.52f, 0.04f, true);
}

void AddTruck(Art* art) {
    AddWheel(art, 0.22f, 0.78f, 0.065f);
    AddWheel(art, 0.4f, 0.78f, 0.065f);
    AddWheel(art, 0.76f, 0.78f, 0.065f);
    AddRect(art, Tone::Deep, 0.08f, 0.66f, 0.84f, 0.08f);
    AddPoly(art, Tone::Body,
            {{0.66f, 0.68f}, {0.92f, 0.68f}, {0.92f, 0.56f}, {0.84f, 0.46f}, {0.66f, 0.46f}});
    AddPoly(art, Tone::Glass, {{0.76f, 0.5f}, {0.83f, 0.5f}, {0.88f, 0.57f}, {0.76f, 0.57f}}, true);
}

void BuildRockets(Art* art) {
    AddTruck(art);
    AddCapsule(art, Tone::Dark, {0.3f, 0.64f}, {0.4f, 0.54f}, 0.05f);
    AddBox(art, Tone::Body, 0.36f, 0.48f, 0.52f, 0.2f, 0.42f);
    // tube mouths on the front face
    for (int i = 0; i < 3; i++) {
        float t = static_cast<float>(i) - 1.0f;
        AddCircle(art, Tone::Dark, 0.6f + t * 0.03f, 0.37f + t * 0.055f, 0.022f, true);
    }
    AddBox(art, Tone::Deep, 0.36f, 0.48f, 0.4f, 0.025f, 0.42f, true);
}

void BuildMissiles(Art* art) {
    AddTruck(art);
    AddBox(art, Tone::Deep, 0.38f, 0.58f, 0.5f, 0.05f, 0.3f);
    AddCapsule(art, Tone::Light, {0.14f, 0.56f}, {0.66f, 0.39f}, 0.1f);
    AddPoly(art, Tone::Body, {{0.62f, 0.35f}, {0.8f, 0.32f}, {0.66f, 0.45f}});
    AddPoly(art, Tone::Body, {{0.12f, 0.5f}, {0.24f, 0.48f}, {0.2f, 0.62f}, {0.1f, 0.62f}});
    AddBox(art, Tone::Body, 0.42f, 0.47f, 0.06f, 0.1f, 0.3f, true);
}

void BuildAntiAir(Art* art) {
    AddTreads(art, 0.12f, 0.8f, 0.75f, 0.15f, 4, 0.04f);
    AddPoly(art, Tone::Body, {{0.1f, 0.68f}, {0.84f, 0.68f}, {0.8f, 0.54f}, {0.14f, 0.54f}});
    AddCapsule(art, Tone::Deep, {0.46f, 0.44f}, {0.72f, 0.14f}, 0.04f);
    AddCapsule(art, Tone::Deep, {0.54f, 0.46f}, {0.8f, 0.16f}, 0.04f);
    AddPoly(art, Tone::Body, {{0.28f, 0.55f}, {0.66f, 0.55f}, {0.66f, 0.4f}, {0.36f, 0.38f}});
    AddRect(art, Tone::Glass, 0.42f, 0.44f, 0.16f, 0.04f, true);
    AddRect(art, Tone::Deep, 0.12f, 0.63f, 0.7f, 0.03f, true);
}

void BuildApc(Art* art) {
    AddTreads(art, 0.1f, 0.88f, 0.75f, 0.16f, 5, 0.04f);
    AddPoly(art, Tone::Body,
            {{0.08f, 0.68f}, {0.92f, 0.68f}, {0.92f, 0.52f}, {0.76f, 0.36f}, {0.1f, 0.36f}});
    AddPoly(art, Tone::Glass, {{0.68f, 0.42f}, {0.77f, 0.42f}, {0.85f, 0.5f}, {0.68f, 0.5f}}, true);
    // a supply cross on the flank
    AddRect(art, Tone::Light, 0.3f, 0.45f, 0.04f, 0.14f, true);
    AddRect(art, Tone::Light, 0.25f, 0.5f, 0.14f, 0.04f, true);
    AddRect(art, Tone::Deep, 0.1f, 0.62f, 0.8f, 0.03f, true);
}

void BuildArt(UnitType type, Art* art) {
    switch (type) {
    case UnitType::Infantry:
        BuildInfantry(art);
        return;
    case UnitType::Mech:
        BuildMech(art);
        return;
    case UnitType::Recon:
        BuildRecon(art);
        return;
    case UnitType::Tank:
        BuildTank(art);
        return;
    case UnitType::MdTank:
        BuildMdTank(art);
        return;
    case UnitType::Neotank:
        BuildNeotank(art);
        return;
    case UnitType::Artillery:
        BuildArtillery(art);
        return;
    case UnitType::Rockets:
        BuildRockets(art);
        return;
    case UnitType::AntiAir:
        BuildAntiAir(art);
        return;
    case UnitType::Missiles:
        BuildMissiles(art);
        return;
    case UnitType::Apc:
        BuildApc(art);
        return;
    case UnitType::Fighter:
        BuildFighter(art);
        return;
    case UnitType::Bomber:
        BuildBomber(art);
        return;
    case UnitType::BCopter:
        BuildBCopter(art);
        return;
    case UnitType::TCopter:
        BuildTCopter(art);
        return;
    case UnitType::Battleship:
        BuildBattleship(art);
        return;
    case UnitType::Cruiser:
        BuildCruiser(art);
        return;
    case UnitType::Lander:
        BuildLander(art);
        return;
    case UnitType::Sub:
        BuildSub(art);
        return;
    case UnitType::Count:
        return;
    }
}

struct Tones {
    Color colors[7];
};

Tones MakeTones(int owner, bool dimmed) {
    Color body = kPlayer[owner & 1];
    Color deep = kPlayerDeep[owner & 1];
    Tones tones;
    tones.colors[static_cast<int>(Tone::Body)] = body;
    tones.colors[static_cast<int>(Tone::Deep)] = deep;
    tones.colors[static_cast<int>(Tone::Light)] = Shade(body, 1.35f);
    tones.colors[static_cast<int>(Tone::Metal)] = Color{0x58, 0x5e, 0x6a};
    tones.colors[static_cast<int>(Tone::Dark)] = Color{0x30, 0x34, 0x3d};
    tones.colors[static_cast<int>(Tone::Glass)] = Color{0xbf, 0xe0, 0xea};
    tones.colors[static_cast<int>(Tone::Face)] = Color{0xf1, 0xd9, 0xbd};
    if (dimmed) {
        // acted units go flat and grey but keep a hint of their side
        Color grey{0x8a, 0x8c, 0x90};
        for (Color& c : tones.colors) {
            c = Shade(Mix(c, grey, 0.4f), 0.72f);
        }
    }
    return tones;
}

void DrawPart(SDL_Renderer* renderer, const Part& part, const Frame& f, bool mirror, float grow,
              Color color) {
    Vec2 points[kMaxPoints];
    for (int i = 0; i < part.count; i++) {
        float fx = mirror ? 1 - part.points[i].x : part.points[i].x;
        points[i] = f.P(fx, part.points[i].y);
    }
    switch (part.kind) {
    case PartKind::Poly: {
        if (grow <= 0) {
            FillPoly(renderer, points, part.count, color);
            return;
        }
        Vec2 grown[kMaxPoints];
        InflatePoly(points, part.count, grow, grown);
        FillPoly(renderer, grown, part.count, color);
        return;
    }
    case PartKind::Circle:
        FillCircle(renderer, points[0], part.radius * f.s + grow, color);
        return;
    case PartKind::Capsule:
        RoundLine(renderer, points[0], points[1], part.radius * 2 * f.s + grow * 2, color);
        return;
    }
}

} // namespace

void DrawUnit(SDL_Renderer* renderer, UnitType type, int owner, float x, float y, float s,
              bool dimmed, bool dived) {
    Art art;
    BuildArt(type, &art);
    Tones tones = MakeTones(owner, dimmed);
    Domain domain = DomainOf(type);
    Frame f{x, y, s};
    bool mirror = owner == 1;
    float outline = s * 0.03f + 0.6f;

    if (domain == Domain::Air) {
        // aloft: a small shadow on the ground, the craft lifted off it
        FillEllipse(renderer, f.P(0.54f, 0.88f), 0.26f * s, 0.045f * s, WithAlpha(kInk, 70));
        f.y -= 0.1f * s;
    } else if (domain == Domain::Sea) {
        FillEllipse(renderer, f.P(0.5f, 0.78f), 0.44f * s, 0.06f * s, WithAlpha(kFoam, 110));
    } else {
        FillEllipse(renderer, f.P(0.5f, 0.84f), 0.36f * s, 0.06f * s, WithAlpha(kInk, 80));
    }
    if (dived) {
        // under water: a dim shape with no outline, the periscope's ripple on top
        for (int i = 0; i < art.count; i++) {
            const Part& part = art.parts[i];
            Color color = Mix(tones.colors[static_cast<int>(part.tone)], kSea, 0.55f);
            DrawPart(renderer, part, f, mirror, 0, WithAlpha(color, 120));
        }
        float px = mirror ? 0.42f : 0.58f;
        FillEllipse(renderer, f.P(px, 0.42f), 0.12f * s, 0.035f * s, WithAlpha(kFoam, 150));
        return;
    }
    // outlines first, so touching parts read as one shape
    for (int i = 0; i < art.count; i++) {
        if (!art.parts[i].detail) {
            DrawPart(renderer, art.parts[i], f, mirror, outline, kInk);
        }
    }
    for (int i = 0; i < art.count; i++) {
        const Part& part = art.parts[i];
        DrawPart(renderer, part, f, mirror, 0, tones.colors[static_cast<int>(part.tone)]);
    }
}

} // namespace shoal
