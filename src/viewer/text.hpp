#pragma once

#include "draw.hpp"

#include <SDL3/SDL.h>
#include <string>
#include <string_view>
#include <vector>

namespace shoal {

constexpr int kFirstGlyph = 32;
constexpr int kGlyphCount = 95; // printable ascii
constexpr int kMaxAtlases = 16;

struct Glyph {
    float src_x = 0;
    float src_y = 0;
    float src_w = 0;
    float src_h = 0;
    float off_x = 0; // from the pen position to the bitmap's top left
    float off_y = 0; // relative to the baseline
    float advance = 0;
};

// one pixel size of the font baked into a texture
struct FontAtlas {
    int pixel_size = 0;
    int last_used = 0;
    SDL_Texture* texture = nullptr;
    float ascent = 0;
    Glyph glyphs[kGlyphCount];
};

struct Font {
    std::vector<unsigned char> data;
    SDL_Renderer* renderer = nullptr;
    FontAtlas atlases[kMaxAtlases];
    int atlas_count = 0;
    int use_clock = 0;
};

bool LoadFont(Font* font, SDL_Renderer* renderer, const std::string& path);
void FreeFont(Font* font);

// y is the top of the line box; returns the advance width
float DrawText(Font* font, float x, float y, float size, Color color, std::string_view text);
float TextWidth(Font* font, float size, std::string_view text);
// pixel height of a line of this size (cap-to-descender box used for layout)
float LineHeight(float size);

enum class Align {
    Left,
    Centre,
    Right,
};

float DrawTextAligned(Font* font, float x, float y, float size, Color color, std::string_view text,
                      Align align);

} // namespace shoal
