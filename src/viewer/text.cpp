#include "text.hpp"

#include "io/map_file.hpp"
#include "stb_truetype.h"

#include <cmath>

namespace shoal {

namespace {

bool BakeAtlas(Font* font, int pixel_size, FontAtlas* atlas) {
    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, font->data.data(),
                        stbtt_GetFontOffsetForIndex(font->data.data(), 0))) {
        return false;
    }
    // grow the bitmap until the printable range fits
    int side = 256;
    std::vector<unsigned char> alpha;
    stbtt_packedchar packed[kGlyphCount];
    while (true) {
        alpha.assign(static_cast<std::size_t>(side * side), 0);
        stbtt_pack_context context;
        if (!stbtt_PackBegin(&context, alpha.data(), side, side, 0, 1, nullptr)) {
            return false;
        }
        int ok = stbtt_PackFontRange(&context, font->data.data(), 0, static_cast<float>(pixel_size),
                                     kFirstGlyph, kGlyphCount, packed);
        stbtt_PackEnd(&context);
        if (ok) {
            break;
        }
        side *= 2;
        if (side > 4096) {
            return false;
        }
    }

    // white pixels carrying the coverage as alpha, tinted per draw with the colour mod
    std::vector<std::uint32_t> pixels(alpha.size());
    for (std::size_t i = 0; i < alpha.size(); i++) {
        pixels[i] = 0x00ffffffu | static_cast<std::uint32_t>(alpha[i]) << 24;
    }
    SDL_Texture* texture = SDL_CreateTexture(font->renderer, SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STATIC, side, side);
    if (!texture) {
        return false;
    }
    SDL_UpdateTexture(texture, nullptr, pixels.data(), side * 4);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
    float scale = stbtt_ScaleForPixelHeight(&info, static_cast<float>(pixel_size));
    atlas->pixel_size = pixel_size;
    atlas->texture = texture;
    atlas->ascent = static_cast<float>(ascent) * scale;
    for (int i = 0; i < kGlyphCount; i++) {
        const stbtt_packedchar& p = packed[i];
        Glyph& glyph = atlas->glyphs[i];
        glyph.src_x = p.x0;
        glyph.src_y = p.y0;
        glyph.src_w = static_cast<float>(p.x1 - p.x0);
        glyph.src_h = static_cast<float>(p.y1 - p.y0);
        glyph.off_x = p.xoff;
        glyph.off_y = p.yoff;
        glyph.advance = p.xadvance;
    }
    return true;
}

FontAtlas* GetAtlas(Font* font, float size) {
    int pixel_size = static_cast<int>(std::lround(size));
    if (pixel_size < 6) {
        pixel_size = 6;
    }
    font->use_clock++;
    for (int i = 0; i < font->atlas_count; i++) {
        if (font->atlases[i].pixel_size == pixel_size) {
            font->atlases[i].last_used = font->use_clock;
            return &font->atlases[i];
        }
    }
    // a free slot, or the least recently used one
    int slot = font->atlas_count;
    if (slot == kMaxAtlases) {
        slot = 0;
        for (int i = 1; i < kMaxAtlases; i++) {
            if (font->atlases[i].last_used < font->atlases[slot].last_used) {
                slot = i;
            }
        }
        SDL_DestroyTexture(font->atlases[slot].texture);
        font->atlases[slot] = FontAtlas{};
    } else {
        font->atlas_count++;
    }
    FontAtlas* atlas = &font->atlases[slot];
    if (!BakeAtlas(font, pixel_size, atlas)) {
        *atlas = FontAtlas{};
        atlas->pixel_size = pixel_size;
        return nullptr;
    }
    atlas->last_used = font->use_clock;
    return atlas;
}

} // namespace

bool LoadFont(Font* font, SDL_Renderer* renderer, const std::string& path) {
    std::string bytes;
    if (!ReadWholeFile(path, &bytes)) {
        return false;
    }
    font->data.assign(bytes.begin(), bytes.end());
    font->renderer = renderer;
    stbtt_fontinfo info;
    return stbtt_InitFont(&info, font->data.data(), 0) != 0;
}

void FreeFont(Font* font) {
    for (int i = 0; i < font->atlas_count; i++) {
        if (font->atlases[i].texture) {
            SDL_DestroyTexture(font->atlases[i].texture);
        }
    }
    font->atlas_count = 0;
}

float LineHeight(float size) {
    return std::round(size * 1.2f);
}

float TextWidth(Font* font, float size, std::string_view text) {
    FontAtlas* atlas = GetAtlas(font, size);
    if (!atlas) {
        return 0;
    }
    float width = 0;
    for (char c : text) {
        int index = static_cast<unsigned char>(c) - kFirstGlyph;
        if (index < 0 || index >= kGlyphCount) {
            index = '?' - kFirstGlyph;
        }
        width += atlas->glyphs[index].advance;
    }
    return width;
}

float DrawText(Font* font, float x, float y, float size, Color color, std::string_view text) {
    FontAtlas* atlas = GetAtlas(font, size);
    if (!atlas) {
        return 0;
    }
    SDL_SetTextureColorMod(atlas->texture, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(atlas->texture, color.a);
    // centre the glyph box in the line box, then snap to whole pixels for crisp stems
    float line = LineHeight(size);
    float baseline =
        std::round(y + (line - static_cast<float>(atlas->pixel_size)) * 0.5f + atlas->ascent);
    float pen = std::round(x);
    for (char c : text) {
        int index = static_cast<unsigned char>(c) - kFirstGlyph;
        if (index < 0 || index >= kGlyphCount) {
            index = '?' - kFirstGlyph;
        }
        const Glyph& glyph = atlas->glyphs[index];
        if (glyph.src_w > 0) {
            SDL_FRect src{glyph.src_x, glyph.src_y, glyph.src_w, glyph.src_h};
            SDL_FRect dst{std::round(pen + glyph.off_x), baseline + std::round(glyph.off_y),
                          glyph.src_w, glyph.src_h};
            SDL_RenderTexture(font->renderer, atlas->texture, &src, &dst);
        }
        pen += glyph.advance;
    }
    return pen - std::round(x);
}

float DrawTextAligned(Font* font, float x, float y, float size, Color color, std::string_view text,
                      Align align) {
    float width = TextWidth(font, size, text);
    if (align == Align::Centre) {
        x -= width * 0.5f;
    } else if (align == Align::Right) {
        x -= width;
    }
    return DrawText(font, x, y, size, color, text);
}

} // namespace shoal
