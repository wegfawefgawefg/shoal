#pragma once

// the pieces unit silhouettes are built from, in 0..1 tile coordinates facing right

#include "draw.hpp"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace shoal {

enum class Tone : std::uint8_t {
    Body,
    Deep,
    Light,
    Metal,
    Dark,
    Glass,
    Face,
};

enum class PartKind : std::uint8_t {
    Poly,
    Circle,
    Capsule,
};

constexpr int kMaxPoints = 8;
constexpr int kMaxParts = 32;

// one piece of a silhouette in 0..1 tile coordinates, facing right
struct Part {
    PartKind kind = PartKind::Poly;
    Tone tone = Tone::Body;
    bool detail = false; // details sit on top and get no outline
    Vec2 points[kMaxPoints];
    int count = 0;
    float radius = 0; // circle radius, capsule half width
};

struct Art {
    Part parts[kMaxParts];
    int count = 0;
};

inline Part* NewPart(Art* art, PartKind kind, Tone tone, bool detail) {
    if (art->count >= kMaxParts) {
        return &art->parts[kMaxParts - 1];
    }
    Part* part = &art->parts[art->count++];
    *part = Part{};
    part->kind = kind;
    part->tone = tone;
    part->detail = detail;
    return part;
}

inline void AddPoly(Art* art, Tone tone, std::initializer_list<Vec2> points, bool detail = false) {
    Part* part = NewPart(art, PartKind::Poly, tone, detail);
    for (Vec2 p : points) {
        if (part->count < kMaxPoints) {
            part->points[part->count++] = p;
        }
    }
}

inline void AddRect(Art* art, Tone tone, float x, float y, float w, float h, bool detail = false) {
    AddPoly(art, tone, {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}}, detail);
}

// a box of length len and height h centred at (cx, cy), tilted up by angle radians
inline void AddBox(Art* art, Tone tone, float cx, float cy, float len, float h, float angle,
                   bool detail = false) {
    float c = std::cos(angle);
    float s = -std::sin(angle);
    float hx = len * 0.5f;
    float hy = h * 0.5f;
    Vec2 corners[4] = {{-hx, -hy}, {hx, -hy}, {hx, hy}, {-hx, hy}};
    Part* part = NewPart(art, PartKind::Poly, tone, detail);
    for (const Vec2& k : corners) {
        part->points[part->count++] = Vec2{cx + k.x * c - k.y * s, cy + k.x * s + k.y * c};
    }
}

inline void AddCircle(Art* art, Tone tone, float cx, float cy, float r, bool detail = false) {
    Part* part = NewPart(art, PartKind::Circle, tone, detail);
    part->points[0] = Vec2{cx, cy};
    part->count = 1;
    part->radius = r;
}

inline void AddCapsule(Art* art, Tone tone, Vec2 a, Vec2 b, float width, bool detail = false) {
    Part* part = NewPart(art, PartKind::Capsule, tone, detail);
    part->points[0] = a;
    part->points[1] = b;
    part->count = 2;
    part->radius = width * 0.5f;
}

inline void AddTreads(Art* art, float x0, float x1, float y, float h, int wheels, float wheel_r) {
    AddCapsule(art, Tone::Dark, {x0 + h * 0.5f, y}, {x1 - h * 0.5f, y}, h);
    for (int i = 0; i < wheels; i++) {
        float t = wheels == 1 ? 0.5f : static_cast<float>(i) / static_cast<float>(wheels - 1);
        float wx = x0 + h * 0.5f + (x1 - x0 - h) * t;
        AddCircle(art, Tone::Metal, wx, y, wheel_r, true);
    }
}

inline void AddWheel(Art* art, float cx, float cy, float r) {
    AddCircle(art, Tone::Dark, cx, cy, r);
    AddCircle(art, Tone::Metal, cx, cy, r * 0.45f, true);
}

// the air and sea silhouettes (unit_art_airsea.cpp)
void BuildFighter(Art* art);
void BuildBomber(Art* art);
void BuildBCopter(Art* art);
void BuildTCopter(Art* art);
void BuildBattleship(Art* art);
void BuildCruiser(Art* art);
void BuildLander(Art* art);
void BuildSub(Art* art);

} // namespace shoal
