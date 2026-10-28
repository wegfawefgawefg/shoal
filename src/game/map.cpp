#include "game/map.hpp"

namespace shoal {

int Neighbours(const Map& map, int tile, int out[4]) {
    int x = TileX(map, tile);
    int y = TileY(map, tile);
    int count = 0;
    if (y > 0) {
        out[count++] = tile - map.width;
    }
    if (x > 0) {
        out[count++] = tile - 1;
    }
    if (x + 1 < map.width) {
        out[count++] = tile + 1;
    }
    if (y + 1 < map.height) {
        out[count++] = tile + map.width;
    }
    return count;
}

void IndexProperties(Map* map) {
    map->property_count = 0;
    int tile_count = map->width * map->height;
    for (int tile = 0; tile < kMaxTiles; tile++) {
        map->property_at[tile] = kNoProperty;
    }
    for (int tile = 0; tile < tile_count; tile++) {
        if (!Spec(map->tiles[tile]).property) {
            continue;
        }
        if (map->property_count >= kMaxProperties) {
            break;
        }
        int index = map->property_count++;
        map->property_tiles[index] = static_cast<std::uint16_t>(tile);
        map->property_at[tile] = static_cast<std::int16_t>(index);
    }
}

std::string TileName(const Map& map, int tile) {
    int x = TileX(map, tile);
    int y = TileY(map, tile);
    std::string name;
    if (x >= 26) {
        name += static_cast<char>('a' + x / 26 - 1);
    }
    name += static_cast<char>('a' + x % 26);
    name += std::to_string(y + 1);
    return name;
}

bool ParseTileName(const Map& map, std::string_view text, int* tile) {
    std::size_t i = 0;
    int x = 0;
    int letters = 0;
    while (i < text.size() && text[i] >= 'a' && text[i] <= 'z') {
        x = x * 26 + (text[i] - 'a' + 1);
        letters++;
        i++;
    }
    if (letters == 0 || letters > 2 || i == text.size()) {
        return false;
    }
    x -= 1;
    int y = 0;
    while (i < text.size()) {
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }
        y = y * 10 + (text[i] - '0');
        i++;
    }
    y -= 1;
    if (x < 0 || y < 0 || x >= map.width || y >= map.height) {
        return false;
    }
    *tile = TileAt(map, x, y);
    return true;
}

} // namespace shoal
