#include "io/board_text.hpp"

namespace shoal {

char UnitLetter(UnitType type) {
    // one letter per unit, picked to stay distinct from terrain glyphs when upper case
    const char letters[kUnitTypeCount] = {'I', 'M', 'R', 'T', 'D', 'N', 'A', 'K', 'F', 'S', 'P'};
    return letters[static_cast<int>(type)];
}

std::string BoardText(const Map& map, const Position& pos) {
    std::string out;
    out += "day " + std::to_string(pos.day) + ", player " + std::to_string(pos.side) +
           " to move, funds " + std::to_string(pos.funds[0]) + " / " +
           std::to_string(pos.funds[1]) + "\n   ";
    for (int x = 0; x < map.width; x++) {
        std::string column = TileName(map, TileAt(map, x, 0));
        column.pop_back();
        while (column.size() < 4) {
            column = " " + column;
        }
        out += column;
    }
    out += "\n";
    for (int y = 0; y < map.height; y++) {
        std::string row = std::to_string(y + 1);
        while (row.size() < 3) {
            row = " " + row;
        }
        for (int x = 0; x < map.width; x++) {
            int tile = TileAt(map, x, y);
            char glyph = Spec(map.tiles[tile]).glyph;
            int prop = map.property_at[tile];
            // owned properties show their owner digit after the glyph
            char owner = ' ';
            if (prop != kNoProperty && pos.prop_owner[prop] != kNeutral) {
                owner = static_cast<char>('0' + pos.prop_owner[prop]);
            }
            std::uint8_t index = pos.unit_at[tile];
            if (index == kNoUnit) {
                row += std::string(" ") + glyph + owner + " ";
                continue;
            }
            const Unit& unit = pos.units[index];
            char letter = UnitLetter(unit.type);
            if (unit.owner == 1) {
                letter = static_cast<char>(letter - 'A' + 'a');
            }
            int hp = DisplayHp(unit.hp);
            char hp_char = hp == 10 ? ' ' : static_cast<char>('0' + hp);
            row += std::string(" ") + glyph + letter + hp_char;
        }
        out += row + "\n";
    }
    return out;
}

} // namespace shoal
