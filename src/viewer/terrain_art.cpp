#include "terrain_art.hpp"

#include "palette.hpp"

namespace shoal {

namespace {

constexpr int kUp = 0;
constexpr int kRight = 1;
constexpr int kDown = 2;
constexpr int kLeft = 3;

bool IsWater(Terrain t) {
    return t == Terrain::Sea || t == Terrain::Reef || t == Terrain::Shoal || t == Terrain::River;
}

bool IsSeaLike(Terrain t) {
    return t == Terrain::Sea || t == Terrain::Reef;
}

// the terrain beside a tile, or Count off the map edge
Terrain Beside(const Map& map, int tile, int dir) {
    int x = TileX(map, tile);
    int y = TileY(map, tile);
    const int dx[4] = {0, 1, 0, -1};
    const int dy[4] = {-1, 0, 1, 0};
    x += dx[dir];
    y += dy[dir];
    if (x < 0 || y < 0 || x >= map.width || y >= map.height) {
        return Terrain::Count;
    }
    return map.tiles[TileAt(map, x, y)];
}

bool RoadJoins(Terrain t) {
    return t == Terrain::Road || t == Terrain::Bridge || Spec(t).property;
}

bool RiverJoins(Terrain t) {
    // off the map edge counts, so rivers run out of the board instead of stopping short
    return t == Terrain::Count || t == Terrain::River || t == Terrain::Sea || t == Terrain::Shoal ||
           t == Terrain::Reef || t == Terrain::Bridge;
}

Color GrassShade(int tile) {
    return Shade(kGrass, 0.97f + 0.06f * TileNoise(tile, 1));
}

void DrawGrass(SDL_Renderer* renderer, const Frame& f, int tile) {
    FillRect(renderer, f.R(0, 0, 1, 1), GrassShade(tile));
    // a few tufts, two blades each
    int tufts = 2 + static_cast<int>(TileNoise(tile, 2) * 2.0f);
    for (int i = 0; i < tufts; i++) {
        float tx = 0.15f + 0.7f * TileNoise(tile, 10 + i);
        float ty = 0.2f + 0.65f * TileNoise(tile, 20 + i);
        Color blade = i % 2 == 0 ? kGrassDark : kGrassLight;
        FillTriangle(renderer, f.P(tx - 0.035f, ty), f.P(tx - 0.005f, ty),
                     f.P(tx - 0.03f, ty - 0.07f), blade);
        FillTriangle(renderer, f.P(tx + 0.005f, ty), f.P(tx + 0.035f, ty),
                     f.P(tx + 0.03f, ty - 0.06f), blade);
    }
}

void DrawConifer(SDL_Renderer* renderer, const Frame& f, float cx, float base, float h) {
    float w = h * 0.42f;
    FillEllipse(renderer, f.P(cx + 0.02f, base + 0.01f), w * f.s * 0.95f, h * f.s * 0.12f,
                WithAlpha(kTreeDark, 110));
    FillRect(renderer, f.R(cx - 0.02f, base - h * 0.18f, 0.04f, h * 0.2f), kTrunk);
    // two tiers, lit from the left
    for (int tier = 0; tier < 2; tier++) {
        float top = base - h + static_cast<float>(tier) * h * 0.32f;
        float bottom = base - h * 0.14f - static_cast<float>(1 - tier) * h * 0.3f;
        float half = w * (0.7f + 0.3f * static_cast<float>(tier));
        FillTriangle(renderer, f.P(cx, top), f.P(cx - half, bottom), f.P(cx, bottom), kTreeLight);
        FillTriangle(renderer, f.P(cx, top), f.P(cx, bottom), f.P(cx + half, bottom), kTree);
    }
}

void DrawWood(SDL_Renderer* renderer, const Frame& f, int tile) {
    DrawGrass(renderer, f, tile);
    float jitter = (TileNoise(tile, 5) - 0.5f) * 0.06f;
    DrawConifer(renderer, f, 0.3f + jitter, 0.52f, 0.42f);
    DrawConifer(renderer, f, 0.7f - jitter, 0.48f, 0.38f);
    DrawConifer(renderer, f, 0.5f, 0.9f, 0.46f);
}

void DrawPeak(SDL_Renderer* renderer, const Frame& f, float left, float right, float apex_x,
              float apex_y, float base) {
    FillTriangle(renderer, f.P(apex_x, apex_y), f.P(left, base), f.P(apex_x + 0.03f, base), kRock);
    FillTriangle(renderer, f.P(apex_x, apex_y), f.P(apex_x + 0.03f, base), f.P(right, base),
                 kRockDark);
    // snow on the top third
    float t = 0.32f;
    float sy = apex_y + (base - apex_y) * t;
    float sl = apex_x + (left - apex_x) * t;
    float sr = apex_x + (right - apex_x) * t;
    Vec2 cap[5] = {f.P(apex_x, apex_y), f.P(sl, sy), f.P(sl + (apex_x - sl) * 0.45f, sy - 0.03f),
                   f.P(apex_x + 0.02f, sy + 0.02f), f.P(sr, sy)};
    FillPoly(renderer, cap, 5, kSnow);
}

void DrawMountain(SDL_Renderer* renderer, const Frame& f, int tile) {
    DrawGrass(renderer, f, tile);
    FillEllipse(renderer, f.P(0.52f, 0.86f), 0.4f * f.s, 0.07f * f.s, WithAlpha(kRockDark, 90));
    DrawPeak(renderer, f, 0.04f, 0.56f, 0.3f, 0.34f, 0.84f);
    DrawPeak(renderer, f, 0.2f, 0.94f, 0.58f, 0.12f, 0.88f);
}

// a band from the tile centre to each joined edge, drawn wide in edge colour then narrow on top
void DrawBand(SDL_Renderer* renderer, const Frame& f, const bool join[4], float width, Color edge,
              Color fill, float edge_width) {
    float half = width * 0.5f;
    for (int pass = 0; pass < 2; pass++) {
        float h = pass == 0 ? half + edge_width : half;
        Color c = pass == 0 ? edge : fill;
        FillRect(renderer, f.R(0.5f - h, 0.5f - h, 2 * h, 2 * h), c);
        if (join[kUp]) {
            FillRect(renderer, f.R(0.5f - h, 0, 2 * h, 0.5f), c);
        }
        if (join[kDown]) {
            FillRect(renderer, f.R(0.5f - h, 0.5f, 2 * h, 0.5f), c);
        }
        if (join[kLeft]) {
            FillRect(renderer, f.R(0, 0.5f - h, 0.5f, 2 * h), c);
        }
        if (join[kRight]) {
            FillRect(renderer, f.R(0.5f, 0.5f - h, 0.5f, 2 * h), c);
        }
    }
}

void DrawRoad(SDL_Renderer* renderer, const Map& map, const Frame& f, int tile) {
    DrawGrass(renderer, f, tile);
    bool join[4];
    int count = 0;
    for (int dir = 0; dir < 4; dir++) {
        join[dir] = RoadJoins(Beside(map, tile, dir));
        count += join[dir] ? 1 : 0;
    }
    if (count == 0) {
        join[kLeft] = join[kRight] = true;
    }
    DrawBand(renderer, f, join, 0.4f, kRoadEdge, kRoad, 0.035f);
}

void DrawRiverBand(SDL_Renderer* renderer, const Frame& f, const bool join[4]) {
    DrawBand(renderer, f, join, 0.42f, kGrassDark, kRiver, 0.03f);
    // a light current down the middle
    bool middle[4] = {join[0], join[1], join[2], join[3]};
    DrawBand(renderer, f, middle, 0.1f, kRiver, kRiverLight, 0.0f);
}

void DrawRiver(SDL_Renderer* renderer, const Map& map, const Frame& f, int tile) {
    DrawGrass(renderer, f, tile);
    bool join[4];
    int count = 0;
    for (int dir = 0; dir < 4; dir++) {
        join[dir] = RiverJoins(Beside(map, tile, dir));
        count += join[dir] ? 1 : 0;
    }
    if (count == 0) {
        join[kUp] = join[kDown] = true;
    }
    DrawRiverBand(renderer, f, join);
}

void DrawWaves(SDL_Renderer* renderer, const Frame& f, int tile) {
    for (int i = 0; i < 2; i++) {
        float wx = 0.22f + 0.56f * TileNoise(tile, 30 + i);
        float wy = 0.25f + 0.5f * TileNoise(tile, 40 + i);
        FillArc(renderer, f.P(wx, wy + 0.08f), 0.08f * f.s, 0.115f * f.s, 3.14159f * 1.2f,
                3.14159f * 1.8f, kSeaLight);
    }
}

// foam where the sea meets anything that is not sea
void DrawCoast(SDL_Renderer* renderer, const Map& map, const Frame& f, int tile) {
    for (int dir = 0; dir < 4; dir++) {
        Terrain t = Beside(map, tile, dir);
        if (t == Terrain::Count || IsSeaLike(t)) {
            continue;
        }
        Color shelf = t == Terrain::Shoal ? kSeaLight : Mix(kSeaLight, kFoam, 0.3f);
        float a = 0.13f;
        float b = 0.05f;
        Rect wide = dir == kUp     ? f.R(0, 0, 1, a)
                    : dir == kDown ? f.R(0, 1 - a, 1, a)
                    : dir == kLeft ? f.R(0, 0, a, 1)
                                   : f.R(1 - a, 0, a, 1);
        Rect thin = dir == kUp     ? f.R(0, 0, 1, b)
                    : dir == kDown ? f.R(0, 1 - b, 1, b)
                    : dir == kLeft ? f.R(0, 0, b, 1)
                                   : f.R(1 - b, 0, b, 1);
        FillRect(renderer, wide, shelf);
        FillRect(renderer, thin, kFoam);
    }
}

void DrawSea(SDL_Renderer* renderer, const Map& map, const Frame& f, int tile) {
    FillRect(renderer, f.R(0, 0, 1, 1), Shade(kSea, 0.98f + 0.04f * TileNoise(tile, 3)));
    DrawWaves(renderer, f, tile);
    DrawCoast(renderer, map, f, tile);
}

// sand reaching to the edges that face water or more beach, rounded where it meets grass
void DrawShoal(SDL_Renderer* renderer, const Map& map, const Frame& f, int tile) {
    bool open[4];
    for (int dir = 0; dir < 4; dir++) {
        Terrain t = Beside(map, tile, dir);
        open[dir] = t == Terrain::Count || IsWater(t) || t == Terrain::Bridge;
    }
    DrawGrass(renderer, f, tile);
    float inset = 0.12f;
    float l = open[kLeft] ? 0 : inset;
    float r = open[kRight] ? 1 : 1 - inset;
    float u = open[kUp] ? 0 : inset;
    float d = open[kDown] ? 1 : 1 - inset;
    FillRect(renderer, f.R(l, u, r - l, d - u), kSand);
    // round the corners where two grass sides meet: grass back in, then a quarter of sand
    float radius = 0.2f;
    float cx[4] = {l, r - radius, r - radius, l};
    float cy[4] = {u, u, d - radius, d - radius};
    bool round[4] = {!open[kUp] && !open[kLeft], !open[kUp] && !open[kRight],
                     !open[kDown] && !open[kRight], !open[kDown] && !open[kLeft]};
    for (int c = 0; c < 4; c++) {
        if (!round[c]) {
            continue;
        }
        FillRect(renderer, f.R(cx[c], cy[c], radius, radius), GrassShade(tile));
        float ox = c == 0 || c == 3 ? cx[c] + radius : cx[c];
        float oy = c == 0 || c == 1 ? cy[c] + radius : cy[c];
        FillCircle(renderer, f.P(ox, oy), radius * f.s, kSand);
    }
    for (int dir = 0; dir < 4; dir++) {
        Terrain t = Beside(map, tile, dir);
        if (t == Terrain::Count || !IsSeaLike(t)) {
            continue;
        }
        // wet sand along the waterline
        Rect band = dir == kUp     ? f.R(0, 0, 1, 0.07f)
                    : dir == kDown ? f.R(0, 0.93f, 1, 0.07f)
                    : dir == kLeft ? f.R(0, 0, 0.07f, 1)
                                   : f.R(0.93f, 0, 0.07f, 1);
        FillRect(renderer, band, kSandDark);
    }
    for (int i = 0; i < 4; i++) {
        float px = 0.25f + 0.5f * TileNoise(tile, 50 + i);
        float py = 0.25f + 0.5f * TileNoise(tile, 60 + i);
        FillCircle(renderer, f.P(px, py), 0.018f * f.s, kSandDark);
    }
}

void DrawRock(SDL_Renderer* renderer, const Frame& f, float cx, float cy, float r) {
    FillEllipse(renderer, f.P(cx, cy + r * 0.5f), r * 1.5f * f.s, r * 0.6f * f.s, kFoam);
    Vec2 rock[5] = {f.P(cx - r, cy + r * 0.4f), f.P(cx - r * 0.6f, cy - r * 0.6f),
                    f.P(cx + r * 0.2f, cy - r), f.P(cx + r, cy - r * 0.1f),
                    f.P(cx + r * 0.8f, cy + r * 0.45f)};
    FillPoly(renderer, rock, 5, kReefRock);
    FillTriangle(renderer, f.P(cx - r * 0.6f, cy - r * 0.6f), f.P(cx + r * 0.2f, cy - r),
                 f.P(cx, cy - r * 0.2f), Shade(kReefRock, 1.25f));
}

void DrawReef(SDL_Renderer* renderer, const Map& map, const Frame& f, int tile) {
    DrawSea(renderer, map, f, tile);
    DrawRock(renderer, f, 0.32f, 0.4f, 0.13f);
    DrawRock(renderer, f, 0.68f, 0.34f, 0.1f);
    DrawRock(renderer, f, 0.55f, 0.7f, 0.15f);
}

void DrawBridge(SDL_Renderer* renderer, const Map& map, const Frame& f, int tile) {
    Terrain around[4];
    bool any_sea = false;
    for (int dir = 0; dir < 4; dir++) {
        around[dir] = Beside(map, tile, dir);
        any_sea = any_sea || IsSeaLike(around[dir]);
    }
    // the deck runs toward roads; the water runs across it
    bool horizontal = true;
    if (RoadJoins(around[kUp]) || RoadJoins(around[kDown])) {
        horizontal = false;
    }
    if (RoadJoins(around[kLeft]) || RoadJoins(around[kRight])) {
        horizontal = true;
    }
    if (any_sea) {
        FillRect(renderer, f.R(0, 0, 1, 1), kSea);
        DrawWaves(renderer, f, tile);
    } else {
        DrawGrass(renderer, f, tile);
        // the water runs across the deck
        bool water[4] = {horizontal, !horizontal, horizontal, !horizontal};
        DrawRiverBand(renderer, f, water);
    }
    if (horizontal) {
        FillRect(renderer, f.R(0, 0.26f, 1, 0.48f), kPlankDark);
        FillRect(renderer, f.R(0, 0.31f, 1, 0.38f), kPlank);
        for (int i = 0; i < 5; i++) {
            FillRect(renderer, f.R(0.1f + 0.2f * static_cast<float>(i), 0.31f, 0.02f, 0.38f),
                     kPlankDark);
        }
    } else {
        FillRect(renderer, f.R(0.26f, 0, 0.48f, 1), kPlankDark);
        FillRect(renderer, f.R(0.31f, 0, 0.38f, 1), kPlank);
        for (int i = 0; i < 5; i++) {
            FillRect(renderer, f.R(0.31f, 0.1f + 0.2f * static_cast<float>(i), 0.38f, 0.02f),
                     kPlankDark);
        }
    }
}

void DrawPad(SDL_Renderer* renderer, const Frame& f, int tile) {
    DrawGrass(renderer, f, tile);
    FillRoundRect(renderer, f.R(0.07f, 0.12f, 0.86f, 0.8f), 0.12f * f.s, kRoadEdge);
    FillRoundRect(renderer, f.R(0.09f, 0.14f, 0.82f, 0.76f), 0.1f * f.s,
                  Mix(kRoad, kGrassLight, 0.25f));
}

} // namespace

float TileNoise(int tile, int salt) {
    std::uint32_t h = static_cast<std::uint32_t>(tile) * 0x9e3779b1u ^
                      static_cast<std::uint32_t>(salt) * 0x85ebca77u;
    h ^= h >> 15;
    h *= 0x2c1b3c6du;
    h ^= h >> 12;
    h *= 0x297a2d39u;
    h ^= h >> 15;
    return static_cast<float>(h & 0xffffu) / 65535.0f;
}

Color OwnerColor(int owner) {
    return owner == 0 || owner == 1 ? kPlayer[owner] : kNeutralRoof;
}

Color OwnerDeep(int owner) {
    return owner == 0 || owner == 1 ? kPlayerDeep[owner] : kNeutralDeep;
}

void DrawTerrain(SDL_Renderer* renderer, const Map& map, int tile, float x, float y, float s) {
    Frame f{x, y, s};
    Terrain t = map.tiles[tile];
    switch (t) {
    case Terrain::Plain:
        DrawGrass(renderer, f, tile);
        return;
    case Terrain::Wood:
        DrawWood(renderer, f, tile);
        return;
    case Terrain::Mountain:
        DrawMountain(renderer, f, tile);
        return;
    case Terrain::Road:
        DrawRoad(renderer, map, f, tile);
        return;
    case Terrain::Bridge:
        DrawBridge(renderer, map, f, tile);
        return;
    case Terrain::River:
        DrawRiver(renderer, map, f, tile);
        return;
    case Terrain::Sea:
        DrawSea(renderer, map, f, tile);
        return;
    case Terrain::Shoal:
        DrawShoal(renderer, map, f, tile);
        return;
    case Terrain::Reef:
        DrawReef(renderer, map, f, tile);
        return;
    case Terrain::City:
    case Terrain::Base:
    case Terrain::Hq:
    case Terrain::Airport:
    case Terrain::Port:
        DrawPad(renderer, f, tile);
        return;
    case Terrain::Count:
        return;
    }
}

} // namespace shoal
