#include "draw.hpp"

#include <cmath>

namespace shoal {

namespace {

constexpr float kPi = 3.14159265f;
constexpr int kMaxFan = 96;

SDL_FColor ToFColor(Color color) {
    return SDL_FColor{color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
}

std::uint8_t ClampByte(float v) {
    if (v < 0) {
        return 0;
    }
    if (v > 255) {
        return 255;
    }
    return static_cast<std::uint8_t>(v + 0.5f);
}

int SegmentsFor(float radius) {
    int segments = static_cast<int>(radius * 0.9f) + 10;
    if (segments > kMaxFan - 2) {
        segments = kMaxFan - 2;
    }
    return segments;
}

} // namespace

Color Mix(Color a, Color b, float t) {
    return Color{ClampByte(a.r + (b.r - a.r) * t), ClampByte(a.g + (b.g - a.g) * t),
                 ClampByte(a.b + (b.b - a.b) * t), ClampByte(a.a + (b.a - a.a) * t)};
}

Color WithAlpha(Color color, int alpha) {
    color.a = ClampByte(static_cast<float>(alpha));
    return color;
}

Color Shade(Color color, float k) {
    if (k <= 1) {
        return Color{ClampByte(color.r * k), ClampByte(color.g * k), ClampByte(color.b * k),
                     color.a};
    }
    Color white{255, 255, 255, color.a};
    return Mix(color, white, k - 1);
}

void FillRect(SDL_Renderer* renderer, Rect rect, Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_FRect r{rect.x, rect.y, rect.w, rect.h};
    SDL_RenderFillRect(renderer, &r);
}

void StrokeRect(SDL_Renderer* renderer, Rect rect, float width, Color color) {
    FillRect(renderer, Rect{rect.x, rect.y, rect.w, width}, color);
    FillRect(renderer, Rect{rect.x, rect.y + rect.h - width, rect.w, width}, color);
    FillRect(renderer, Rect{rect.x, rect.y + width, width, rect.h - 2 * width}, color);
    FillRect(renderer, Rect{rect.x + rect.w - width, rect.y + width, width, rect.h - 2 * width},
             color);
}

void FillPoly(SDL_Renderer* renderer, const Vec2* points, int count, Color color) {
    if (count < 3 || count > kMaxFan) {
        return;
    }
    SDL_Vertex verts[kMaxFan];
    int indices[(kMaxFan - 2) * 3];
    SDL_FColor fcolor = ToFColor(color);
    for (int i = 0; i < count; i++) {
        verts[i].position = SDL_FPoint{points[i].x, points[i].y};
        verts[i].color = fcolor;
        verts[i].tex_coord = SDL_FPoint{0, 0};
    }
    int index_count = 0;
    for (int i = 1; i + 1 < count; i++) {
        indices[index_count++] = 0;
        indices[index_count++] = i;
        indices[index_count++] = i + 1;
    }
    SDL_RenderGeometry(renderer, nullptr, verts, count, indices, index_count);
}

void FillTriangle(SDL_Renderer* renderer, Vec2 a, Vec2 b, Vec2 c, Color color) {
    Vec2 points[3] = {a, b, c};
    FillPoly(renderer, points, 3, color);
}

void FillEllipse(SDL_Renderer* renderer, Vec2 centre, float rx, float ry, Color color) {
    int segments = SegmentsFor(rx > ry ? rx : ry);
    Vec2 points[kMaxFan];
    for (int i = 0; i < segments; i++) {
        float angle = 2 * kPi * static_cast<float>(i) / static_cast<float>(segments);
        points[i] = Vec2{centre.x + std::cos(angle) * rx, centre.y + std::sin(angle) * ry};
    }
    FillPoly(renderer, points, segments, color);
}

void FillCircle(SDL_Renderer* renderer, Vec2 centre, float radius, Color color) {
    FillEllipse(renderer, centre, radius, radius, color);
}

void FillRoundRect(SDL_Renderer* renderer, Rect rect, float radius, Color color) {
    float limit = (rect.w < rect.h ? rect.w : rect.h) * 0.5f;
    if (radius > limit) {
        radius = limit;
    }
    if (radius <= 0.5f) {
        FillRect(renderer, rect, color);
        return;
    }
    // one convex outline: four quarter circles joined by the straight sides
    constexpr int kCorner = 8;
    Vec2 points[kCorner * 4];
    Vec2 centres[4] = {{rect.x + rect.w - radius, rect.y + rect.h - radius},
                       {rect.x + radius, rect.y + rect.h - radius},
                       {rect.x + radius, rect.y + radius},
                       {rect.x + rect.w - radius, rect.y + radius}};
    int count = 0;
    for (int corner = 0; corner < 4; corner++) {
        for (int i = 0; i < kCorner; i++) {
            float t = static_cast<float>(i) / static_cast<float>(kCorner - 1);
            float angle = (static_cast<float>(corner) + t) * kPi * 0.5f;
            points[count++] = Vec2{centres[corner].x + std::cos(angle) * radius,
                                   centres[corner].y + std::sin(angle) * radius};
        }
    }
    FillPoly(renderer, points, count, color);
}

void Line(SDL_Renderer* renderer, Vec2 a, Vec2 b, float width, Color color) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.001f) {
        return;
    }
    float nx = -dy / len * width * 0.5f;
    float ny = dx / len * width * 0.5f;
    Vec2 points[4] = {
        {a.x + nx, a.y + ny}, {b.x + nx, b.y + ny}, {b.x - nx, b.y - ny}, {a.x - nx, a.y - ny}};
    FillPoly(renderer, points, 4, color);
}

void RoundLine(SDL_Renderer* renderer, Vec2 a, Vec2 b, float width, Color color) {
    Line(renderer, a, b, width, color);
    FillCircle(renderer, a, width * 0.5f, color);
    FillCircle(renderer, b, width * 0.5f, color);
}

void FillArc(SDL_Renderer* renderer, Vec2 centre, float r_in, float r_out, float a0, float a1,
             Color color) {
    constexpr int kSteps = 32;
    SDL_Vertex verts[(kSteps + 1) * 2];
    int indices[kSteps * 6];
    SDL_FColor fcolor = ToFColor(color);
    for (int i = 0; i <= kSteps; i++) {
        float angle = a0 + (a1 - a0) * static_cast<float>(i) / kSteps;
        float c = std::cos(angle);
        float s = std::sin(angle);
        verts[i * 2].position = SDL_FPoint{centre.x + c * r_in, centre.y + s * r_in};
        verts[i * 2 + 1].position = SDL_FPoint{centre.x + c * r_out, centre.y + s * r_out};
        verts[i * 2].color = fcolor;
        verts[i * 2 + 1].color = fcolor;
        verts[i * 2].tex_coord = SDL_FPoint{0, 0};
        verts[i * 2 + 1].tex_coord = SDL_FPoint{0, 0};
    }
    for (int i = 0; i < kSteps; i++) {
        int base = i * 2;
        int* out = indices + i * 6;
        out[0] = base;
        out[1] = base + 1;
        out[2] = base + 3;
        out[3] = base;
        out[4] = base + 3;
        out[5] = base + 2;
    }
    SDL_RenderGeometry(renderer, nullptr, verts, (kSteps + 1) * 2, indices, kSteps * 6);
}

void InflatePoly(const Vec2* points, int count, float amount, Vec2* out) {
    // winding decides which side of each edge is outside
    float area = 0;
    for (int i = 0; i < count; i++) {
        const Vec2& a = points[i];
        const Vec2& b = points[(i + 1) % count];
        area += a.x * b.y - b.x * a.y;
    }
    float side = area > 0 ? 1.0f : -1.0f;
    for (int i = 0; i < count; i++) {
        const Vec2& prev = points[(i + count - 1) % count];
        const Vec2& cur = points[i];
        const Vec2& next = points[(i + 1) % count];
        float e0x = cur.x - prev.x;
        float e0y = cur.y - prev.y;
        float e1x = next.x - cur.x;
        float e1y = next.y - cur.y;
        float l0 = std::sqrt(e0x * e0x + e0y * e0y) + 1e-6f;
        float l1 = std::sqrt(e1x * e1x + e1y * e1y) + 1e-6f;
        // outward normals of the two edges meeting here
        float n0x = e0y / l0 * side;
        float n0y = -e0x / l0 * side;
        float n1x = e1y / l1 * side;
        float n1y = -e1x / l1 * side;
        float mx = n0x + n1x;
        float my = n0y + n1y;
        float ml = std::sqrt(mx * mx + my * my);
        if (ml < 1e-4f) {
            out[i] = Vec2{cur.x + n0x * amount, cur.y + n0y * amount};
            continue;
        }
        mx /= ml;
        my /= ml;
        // stretch so the edges move by amount, capped at sharp corners
        float cos_half = mx * n0x + my * n0y;
        float stretch = cos_half > 0.35f ? 1.0f / cos_half : 1.0f / 0.35f;
        out[i] = Vec2{cur.x + mx * amount * stretch, cur.y + my * amount * stretch};
    }
}

} // namespace shoal
