#pragma once

#include <SDL3/SDL.h>
#include <cstdint>

namespace shoal {

struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
};

struct Vec2 {
    float x = 0;
    float y = 0;
};

struct Rect {
    float x = 0;
    float y = 0;
    float w = 0;
    float h = 0;
};

inline bool Contains(const Rect& rect, float x, float y) {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
}

// a tile's corner and size; P and R map 0..1 tile coordinates to pixels
struct Frame {
    float x = 0;
    float y = 0;
    float s = 0;

    Vec2 P(float fx, float fy) const {
        return Vec2{x + fx * s, y + fy * s};
    }
    Rect R(float fx, float fy, float fw, float fh) const {
        return Rect{x + fx * s, y + fy * s, fw * s, fh * s};
    }
};

// t = 0 gives a, t = 1 gives b
Color Mix(Color a, Color b, float t);
Color WithAlpha(Color color, int alpha);
// k < 1 darkens toward black, k > 1 lightens toward white
Color Shade(Color color, float k);

void FillRect(SDL_Renderer* renderer, Rect rect, Color color);
void StrokeRect(SDL_Renderer* renderer, Rect rect, float width, Color color);
void FillRoundRect(SDL_Renderer* renderer, Rect rect, float radius, Color color);
void FillCircle(SDL_Renderer* renderer, Vec2 centre, float radius, Color color);
void FillEllipse(SDL_Renderer* renderer, Vec2 centre, float rx, float ry, Color color);
// convex polygons only; drawn as a fan from the first point
void FillPoly(SDL_Renderer* renderer, const Vec2* points, int count, Color color);
void FillTriangle(SDL_Renderer* renderer, Vec2 a, Vec2 b, Vec2 c, Color color);
// a straight band of the given width; round adds circular caps
void Line(SDL_Renderer* renderer, Vec2 a, Vec2 b, float width, Color color);
void RoundLine(SDL_Renderer* renderer, Vec2 a, Vec2 b, float width, Color color);
// a band between two radii, from angle a0 to a1 in radians (0 = right, clockwise on screen)
void FillArc(SDL_Renderer* renderer, Vec2 centre, float r_in, float r_out, float a0, float a1,
             Color color);

// pushes the outline of a convex polygon outward by amount
void InflatePoly(const Vec2* points, int count, float amount, Vec2* out);

} // namespace shoal
