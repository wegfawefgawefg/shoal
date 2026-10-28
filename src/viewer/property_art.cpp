#include "palette.hpp"
#include "terrain_art.hpp"

#include <cmath>

namespace shoal {

namespace {

// buildings get a thin dark rim so they hold up against the pad and the grass
void Block(SDL_Renderer* renderer, const Frame& f, float x, float y, float w, float h, Color fill) {
    float rim = 0.025f;
    FillRect(renderer, f.R(x - rim, y - rim, w + 2 * rim, h + 2 * rim), kInk);
    FillRect(renderer, f.R(x, y, w, h), fill);
}

void Windows(SDL_Renderer* renderer, const Frame& f, float x, float y, float w, float h, int cols,
             int rows) {
    float gap_x = w / static_cast<float>(cols);
    float gap_y = h / static_cast<float>(rows);
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            float wx = x + gap_x * (static_cast<float>(c) + 0.25f);
            float wy = y + gap_y * (static_cast<float>(r) + 0.25f);
            FillRect(renderer, f.R(wx, wy, gap_x * 0.5f, gap_y * 0.5f), kWindow);
        }
    }
}

void Shadow(SDL_Renderer* renderer, const Frame& f, float cx, float cy, float rx) {
    FillEllipse(renderer, f.P(cx, cy), rx * f.s, 0.06f * f.s, WithAlpha(kInk, 70));
}

void DrawCity(SDL_Renderer* renderer, const Frame& f, Color roof) {
    Shadow(renderer, f, 0.5f, 0.84f, 0.38f);
    // a tall block behind, a low block in front
    Block(renderer, f, 0.18f, 0.24f, 0.34f, 0.58f, kWallShade);
    Block(renderer, f, 0.18f, 0.24f, 0.34f, 0.12f, roof);
    Windows(renderer, f, 0.18f, 0.38f, 0.34f, 0.42f, 2, 3);
    Block(renderer, f, 0.46f, 0.46f, 0.36f, 0.38f, kWall);
    Block(renderer, f, 0.46f, 0.46f, 0.36f, 0.1f, roof);
    Windows(renderer, f, 0.46f, 0.58f, 0.36f, 0.24f, 3, 2);
}

void DrawBase(SDL_Renderer* renderer, const Frame& f, Color roof) {
    Shadow(renderer, f, 0.5f, 0.85f, 0.4f);
    // chimney first so the hall overlaps its foot
    Block(renderer, f, 0.66f, 0.14f, 0.1f, 0.4f, kWallShade);
    FillRect(renderer, f.R(0.66f, 0.2f, 0.1f, 0.06f), roof);
    Block(renderer, f, 0.12f, 0.5f, 0.76f, 0.34f, kWall);
    // sawtooth roof: three teeth in the owner colour
    for (int i = 0; i < 3; i++) {
        float x0 = 0.12f + 0.2533f * static_cast<float>(i);
        float x1 = x0 + 0.2533f;
        Vec2 rim[3] = {f.P(x0 - 0.02f, 0.52f), f.P(x1 + 0.01f, 0.52f), f.P(x1 + 0.01f, 0.33f)};
        FillPoly(renderer, rim, 3, kInk);
        FillTriangle(renderer, f.P(x0, 0.5f), f.P(x1, 0.5f), f.P(x1, 0.36f), roof);
    }
    FillRect(renderer, f.R(0.38f, 0.64f, 0.24f, 0.2f), kWindow);
    for (int i = 0; i < 3; i++) {
        FillRect(renderer, f.R(0.38f, 0.68f + 0.05f * static_cast<float>(i), 0.24f, 0.015f),
                 Shade(kWindow, 1.4f));
    }
}

void DrawHq(SDL_Renderer* renderer, const Frame& f, Color roof, Color deep) {
    Shadow(renderer, f, 0.5f, 0.86f, 0.42f);
    // wings, then the keep with its battlements, then the flag
    Block(renderer, f, 0.12f, 0.56f, 0.76f, 0.28f, kWallShade);
    Block(renderer, f, 0.32f, 0.3f, 0.36f, 0.54f, kWall);
    for (int i = 0; i < 3; i++) {
        Block(renderer, f, 0.32f + 0.14f * static_cast<float>(i), 0.24f, 0.08f, 0.07f, kWall);
    }
    FillRect(renderer, f.R(0.32f, 0.38f, 0.36f, 0.08f), roof);
    FillRect(renderer, f.R(0.12f, 0.56f, 0.2f, 0.06f), roof);
    FillRect(renderer, f.R(0.68f, 0.56f, 0.2f, 0.06f), roof);
    // arched door
    FillRect(renderer, f.R(0.43f, 0.66f, 0.14f, 0.18f), kWindow);
    FillCircle(renderer, f.P(0.5f, 0.66f), 0.07f * f.s, kWindow);
    // flag
    FillRect(renderer, f.R(0.49f, 0.02f, 0.025f, 0.24f), kInk);
    Vec2 flag[4] = {f.P(0.515f, 0.03f), f.P(0.8f, 0.07f), f.P(0.74f, 0.11f), f.P(0.515f, 0.15f)};
    Vec2 edge[4];
    InflatePoly(flag, 4, 0.018f * f.s, edge);
    FillPoly(renderer, edge, 4, kInk);
    FillPoly(renderer, flag, 4, roof);
    FillTriangle(renderer, f.P(0.515f, 0.09f), f.P(0.74f, 0.11f), f.P(0.515f, 0.15f), deep);
}

// the top half of an ellipse standing on y = base
void Dome(SDL_Renderer* renderer, const Frame& f, float cx, float base, float rx, float ry,
          Color color) {
    constexpr int kSteps = 16;
    Vec2 points[kSteps + 1];
    for (int i = 0; i <= kSteps; i++) {
        float a = 3.14159265f * static_cast<float>(i) / kSteps;
        points[i] = f.P(cx - std::cos(a) * rx, base - std::sin(a) * ry);
    }
    FillPoly(renderer, points, kSteps + 1, color);
}

void DrawAirport(SDL_Renderer* renderer, const Frame& f, Color roof) {
    Shadow(renderer, f, 0.5f, 0.84f, 0.4f);
    // runway stripe, hangar dome, control tower
    FillRect(renderer, f.R(0.08f, 0.74f, 0.84f, 0.12f), kWindow);
    for (int i = 0; i < 4; i++) {
        FillRect(renderer, f.R(0.13f + 0.2f * static_cast<float>(i), 0.795f, 0.1f, 0.025f), kWall);
    }
    Dome(renderer, f, 0.38f, 0.74f, 0.29f, 0.4f, kInk);
    Dome(renderer, f, 0.38f, 0.74f, 0.265f, 0.375f, kWall);
    FillRect(renderer, f.R(0.115f, 0.5f, 0.53f, 0.07f), roof);
    FillRect(renderer, f.R(0.27f, 0.6f, 0.22f, 0.14f), kWindow);
    Block(renderer, f, 0.73f, 0.34f, 0.08f, 0.4f, kWallShade);
    Block(renderer, f, 0.68f, 0.24f, 0.18f, 0.11f, roof);
    FillRect(renderer, f.R(0.7f, 0.265f, 0.14f, 0.045f), kWindow);
}

void DrawPort(SDL_Renderer* renderer, const Frame& f, Color roof) {
    // a quay on the water, a crane over it
    FillRect(renderer, f.R(0.06f, 0.7f, 0.88f, 0.2f), kSea);
    Block(renderer, f, 0.06f, 0.6f, 0.88f, 0.12f, kWallShade);
    Block(renderer, f, 0.12f, 0.44f, 0.22f, 0.16f, roof);
    Block(renderer, f, 0.36f, 0.48f, 0.18f, 0.12f, kWall);
    Block(renderer, f, 0.66f, 0.16f, 0.06f, 0.44f, roof);
    Block(renderer, f, 0.3f, 0.14f, 0.5f, 0.05f, roof);
    FillRect(renderer, f.R(0.355f, 0.19f, 0.012f, 0.18f), kInk);
    FillRect(renderer, f.R(0.335f, 0.37f, 0.05f, 0.03f), kInk);
}

} // namespace

void DrawProperty(SDL_Renderer* renderer, Terrain terrain, int owner, float x, float y, float s) {
    Frame f{x, y, s};
    Color roof = OwnerColor(owner);
    switch (terrain) {
    case Terrain::City:
        DrawCity(renderer, f, roof);
        return;
    case Terrain::Base:
        DrawBase(renderer, f, roof);
        return;
    case Terrain::Hq:
        DrawHq(renderer, f, roof, OwnerDeep(owner));
        return;
    case Terrain::Airport:
        DrawAirport(renderer, f, roof);
        return;
    case Terrain::Port:
        DrawPort(renderer, f, roof);
        return;
    default:
        return;
    }
}

} // namespace shoal
