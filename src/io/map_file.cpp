#include "io/map_file.hpp"

#include <charconv>
#include <fstream>
#include <gsexp/sexp.hpp>
#include <sstream>

namespace shoal {

namespace {

bool ParseInt(gsexp::Node node, int* out) {
    if (!node.valid() || node.is_list()) {
        return false;
    }
    std::string_view text = node.text();
    auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), *out);
    return ec == std::errc() && end == text.data() + text.size();
}

bool Fail(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
    return false;
}

bool ReadTiles(gsexp::Node form, Map* map, std::string* error) {
    int y = 0;
    for (gsexp::Node row : form.children()) {
        if (row.is_atom("tiles")) {
            continue;
        }
        if (!row.is_string()) {
            return Fail(error, "tiles rows must be strings");
        }
        std::string_view text = row.text();
        int x = 0;
        for (char c : text) {
            if (c == ' ') {
                continue;
            }
            Terrain terrain;
            if (!TerrainFromGlyph(c, &terrain)) {
                return Fail(error, std::string("unknown terrain glyph '") + c + "'");
            }
            if (x >= kMaxSide || y >= kMaxSide) {
                return Fail(error, "map is larger than 32x32");
            }
            map->tiles[y * kMaxSide + x] = terrain;
            x++;
        }
        if (y == 0) {
            map->width = x;
        } else if (x != map->width) {
            return Fail(error, "tiles row " + std::to_string(y + 1) + " has a different width");
        }
        y++;
    }
    map->height = y;
    // repack from the 32-wide scratch layout to width-wide rows
    Terrain packed[kMaxTiles] = {};
    for (int ty = 0; ty < map->height; ty++) {
        for (int tx = 0; tx < map->width; tx++) {
            packed[ty * map->width + tx] = map->tiles[ty * kMaxSide + tx];
        }
    }
    for (int i = 0; i < kMaxTiles; i++) {
        map->tiles[i] = packed[i];
    }
    if (map->width == 0 || map->height == 0) {
        return Fail(error, "map has no tiles");
    }
    IndexProperties(map);
    for (int prop = 0; prop < map->property_count; prop++) {
        map->start_owner[prop] = kNeutral;
    }
    return true;
}

bool ReadOwner(gsexp::Node form, Map* map, std::string* error) {
    int owner = 0;
    if (!ParseInt(form.child_at(1), &owner) || owner < 0 || owner > 1) {
        return Fail(error, "owner wants 0 or 1 then tiles");
    }
    for (std::size_t i = 2; i < form.child_count(); i++) {
        int tile = 0;
        std::string_view name = form.child_at(i).text();
        if (!ParseTileName(*map, name, &tile) || map->property_at[tile] == kNoProperty) {
            return Fail(error, "owner tile " + std::string(name) + " is not a property");
        }
        map->start_owner[map->property_at[tile]] = static_cast<std::uint8_t>(owner);
    }
    return true;
}

bool ReadUnit(gsexp::Node form, Map* map, std::string* error) {
    StartUnit unit;
    UnitType type;
    int tile = 0;
    if (form.child_count() < 4 || !ParseInt(form.child_at(1), &unit.owner) ||
        !UnitTypeFromShortName(form.child_at(2).text(), &type) ||
        !ParseTileName(*map, form.child_at(3).text(), &tile)) {
        return Fail(error, "unit wants (unit owner type tile [hp [fuel]])");
    }
    unit.type = type;
    unit.tile = tile;
    if (form.child_count() >= 5 && !ParseInt(form.child_at(4), &unit.hp)) {
        return Fail(error, "unit hp must be a number");
    }
    if (form.child_count() >= 6 &&
        (!ParseInt(form.child_at(5), &unit.fuel) || unit.fuel < 0 || unit.fuel > Spec(type).fuel)) {
        return Fail(error, "unit fuel must be 0 up to a full tank");
    }
    if (unit.owner < 0 || unit.owner > 1 || unit.hp < 1 || unit.hp > 100) {
        return Fail(error, "unit owner must be 0/1 and hp 1..100");
    }
    for (const StartUnit& other : map->start_units) {
        if (other.tile == tile) {
            return Fail(error, "two units on " + TileName(*map, tile));
        }
    }
    if (MoveCost(map->tiles[tile], Spec(type).move_type) == 0) {
        return Fail(error, "unit cannot stand on " + TileName(*map, tile));
    }
    map->start_units.push_back(unit);
    return true;
}

bool ReadMap(gsexp::Node root, Map* map, std::string* error) {
    if (!root.is_list() || !root.head().is_atom("map")) {
        return Fail(error, "expected (map ...)");
    }
    *map = Map{};
    gsexp::FormView view(root);
    gsexp::Node tiles = view.find("tiles");
    if (!tiles.valid()) {
        return Fail(error, "map needs (tiles ...)");
    }
    if (!ReadTiles(tiles, map, error)) {
        return false;
    }
    for (gsexp::Node form : root.children()) {
        if (!form.is_list() || form.child_count() == 0) {
            continue;
        }
        gsexp::Node head = form.head();
        bool ok = true;
        if (head.is_atom("name")) {
            map->name = std::string(form.child_at(1).text());
        } else if (head.is_atom("day-limit")) {
            ok = ParseInt(form.child_at(1), &map->day_limit) && map->day_limit >= 1;
        } else if (head.is_atom("funds")) {
            ok = ParseInt(form.child_at(1), &map->start_funds[0]) &&
                 ParseInt(form.child_at(2), &map->start_funds[1]);
        } else if (head.is_atom("income")) {
            ok = ParseInt(form.child_at(1), &map->funds_per_property);
        } else if (head.is_atom("build")) {
            map->production = true;
            for (std::size_t i = 1; i < form.child_count(); i++) {
                UnitType type;
                if (!UnitTypeFromShortName(form.child_at(i).text(), &type)) {
                    return Fail(error, "unknown unit " + std::string(form.child_at(i).text()));
                }
                map->buildable[static_cast<int>(type)] = true;
            }
        } else if (head.is_atom("owner")) {
            if (!ReadOwner(form, map, error)) {
                return false;
            }
        } else if (head.is_atom("unit")) {
            if (!ReadUnit(form, map, error)) {
                return false;
            }
        }
        if (!ok) {
            return Fail(error, "bad (" + std::string(head.text()) + " ...)");
        }
    }
    if (static_cast<int>(map->start_units.size()) > kMaxUnits) {
        return Fail(error, "more than " + std::to_string(kMaxUnits) + " units");
    }
    return true;
}

void WriteMapForm(const Map& map, std::ostringstream& out, const char* indent) {
    out << indent << "(map\n";
    out << indent << "  (name " << gsexp::quote_string(map.name) << ")\n";
    out << indent << "  (day-limit " << map.day_limit << ")\n";
    out << indent << "  (funds " << map.start_funds[0] << " " << map.start_funds[1] << ")\n";
    out << indent << "  (income " << map.funds_per_property << ")\n";
    if (map.production) {
        out << indent << "  (build";
        for (int type = 0; type < kUnitTypeCount; type++) {
            if (map.buildable[type]) {
                out << " " << Spec(static_cast<UnitType>(type)).short_name;
            }
        }
        out << ")\n";
    }
    out << indent << "  (tiles";
    for (int y = 0; y < map.height; y++) {
        std::string row;
        for (int x = 0; x < map.width; x++) {
            row += Spec(map.tiles[TileAt(map, x, y)]).glyph;
        }
        out << "\n" << indent << "    \"" << row << "\"";
    }
    out << ")\n";
    for (int owner = 0; owner < 2; owner++) {
        std::string tiles;
        for (int prop = 0; prop < map.property_count; prop++) {
            if (map.start_owner[prop] == owner) {
                tiles += " " + TileName(map, map.property_tiles[prop]);
            }
        }
        if (!tiles.empty()) {
            out << indent << "  (owner " << owner << tiles << ")\n";
        }
    }
    for (const StartUnit& unit : map.start_units) {
        out << indent << "  (unit " << unit.owner << " " << Spec(unit.type).short_name << " "
            << TileName(map, unit.tile);
        if (unit.hp != 100 || unit.fuel >= 0) {
            out << " " << unit.hp;
        }
        if (unit.fuel >= 0) {
            out << " " << unit.fuel;
        }
        out << ")\n";
    }
    out << indent << ")";
}

struct MarkName {
    TurnMark mark;
    const char* name;
    const char* symbol;
};

constexpr MarkName kMarkNames[] = {
    {TurnMark::None, "none", ""},
    {TurnMark::Inaccuracy, "inaccuracy", "?!"},
    {TurnMark::Mistake, "mistake", "?"},
    {TurnMark::Blunder, "blunder", "??"},
};

// (turn PLY SCORE LOSS MARK "moves, ..."); the best turn must be legal from its ply
bool ReadTurnAnalysis(gsexp::Node form, const Map& map, const std::vector<Position>& positions,
                      TurnAnalysis* turn, std::string* error) {
    if (form.child_count() != 6 || !ParseInt(form.child_at(1), &turn->ply) ||
        !ParseInt(form.child_at(2), &turn->score) || !ParseInt(form.child_at(3), &turn->loss) ||
        !TurnMarkFromName(form.child_at(4).text(), &turn->mark) || !form.child_at(5).is_string()) {
        return Fail(error, "analysis wants (turn ply score loss mark \"moves\")");
    }
    if (turn->ply < 0 || turn->ply >= static_cast<int>(positions.size())) {
        return Fail(error, "analysis ply " + std::to_string(turn->ply) + " is past the moves");
    }
    Position pos = positions[static_cast<std::size_t>(turn->ply)];
    std::string_view text = form.child_at(5).text();
    while (!text.empty()) {
        std::size_t comma = text.find(',');
        std::string_view part = text.substr(0, comma);
        text = comma == std::string_view::npos ? std::string_view() : text.substr(comma + 1);
        while (!part.empty() && part.front() == ' ') {
            part.remove_prefix(1);
        }
        Move move;
        if (!ParseMoveText(map, part, &move) || !IsLegal(map, pos, move)) {
            return Fail(error, "analysis at ply " + std::to_string(turn->ply) + ": bad move " +
                                   std::string(part));
        }
        ApplyMove(map, &pos, move);
        turn->best.push_back(move);
    }
    return true;
}

} // namespace

const char* TurnMarkName(TurnMark mark) {
    return kMarkNames[static_cast<int>(mark)].name;
}

const char* TurnMarkSymbol(TurnMark mark) {
    return kMarkNames[static_cast<int>(mark)].symbol;
}

bool TurnMarkFromName(std::string_view name, TurnMark* mark) {
    for (const MarkName& entry : kMarkNames) {
        if (name == entry.name) {
            *mark = entry.mark;
            return true;
        }
    }
    return false;
}

std::string TurnMovesText(const Map& map, const std::vector<Move>& moves) {
    std::string text;
    for (const Move& move : moves) {
        if (!text.empty()) {
            text += ", ";
        }
        text += MoveText(map, move);
    }
    return text;
}

bool ReadWholeFile(const std::string& path, std::string* out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    std::ostringstream text;
    text << file.rdbuf();
    *out = text.str();
    return true;
}

bool WriteWholeFile(const std::string& path, std::string_view text) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    return static_cast<bool>(file);
}

bool LoadMapText(std::string_view text, Map* map, std::string* error) {
    gsexp::ParseResult parsed = gsexp::parse(text);
    if (!parsed.ok || parsed.root_count() == 0) {
        std::string message = "parse error";
        if (!parsed.diagnostics.empty()) {
            const gsexp::Diagnostic& d = parsed.diagnostics[0];
            message +=
                " at " + std::to_string(d.line) + ":" + std::to_string(d.column) + ": " + d.message;
        }
        return Fail(error, message);
    }
    return ReadMap(parsed.root(0), map, error);
}

bool LoadMapFile(const std::string& path, Map* map, std::string* error) {
    std::string text;
    if (!ReadWholeFile(path, &text)) {
        return Fail(error, "cannot read " + path);
    }
    return LoadMapText(text, map, error);
}

std::string WriteMapText(const Map& map) {
    std::ostringstream out;
    WriteMapForm(map, out, "");
    out << "\n";
    return out.str();
}

bool LoadGameText(std::string_view text, GameRecord* game, std::string* error) {
    gsexp::ParseResult parsed = gsexp::parse(text);
    if (!parsed.ok || parsed.root_count() == 0) {
        return Fail(error, "parse error");
    }
    gsexp::Node root = parsed.root(0);
    if (!root.is_list() || !root.head().is_atom("game")) {
        return Fail(error, "expected (game ...)");
    }
    gsexp::FormView view(root);
    if (!ReadMap(view.find("map"), &game->map, error)) {
        return false;
    }
    game->moves.clear();
    game->evals.clear();
    game->analysis.clear();
    // replay as we read so a bad move is caught at its index
    Position pos;
    SetupStart(game->map, &pos);
    std::vector<Position> positions{pos};
    gsexp::Node moves = view.find("moves");
    if (moves.valid()) {
        for (std::size_t i = 1; i < moves.child_count(); i++) {
            Move move;
            std::string_view move_text = moves.child_at(i).text();
            if (!ParseMoveText(game->map, move_text, &move)) {
                return Fail(error, "cannot read move " + std::to_string(i) + ": " +
                                       std::string(move_text));
            }
            if (!IsLegal(game->map, pos, move)) {
                return Fail(error,
                            "illegal move " + std::to_string(i) + ": " + std::string(move_text));
            }
            ApplyMove(game->map, &pos, move);
            game->moves.push_back(move);
            positions.push_back(pos);
        }
    }
    gsexp::Node evals = view.find("evals");
    if (evals.valid()) {
        for (std::size_t i = 1; i < evals.child_count(); i++) {
            int eval = 0;
            if (!ParseInt(evals.child_at(i), &eval)) {
                return Fail(error, "evals must be numbers");
            }
            game->evals.push_back(eval);
        }
    }
    gsexp::Node analysis = view.find("analysis");
    if (analysis.valid()) {
        for (std::size_t i = 1; i < analysis.child_count(); i++) {
            gsexp::Node form = analysis.child_at(i);
            if (!form.is_list() || form.child_count() == 0 || !form.head().is_atom("turn")) {
                return Fail(error, "analysis holds (turn ...) forms");
            }
            TurnAnalysis turn;
            if (!ReadTurnAnalysis(form, game->map, positions, &turn, error)) {
                return false;
            }
            game->analysis.push_back(std::move(turn));
        }
    }
    return true;
}

bool LoadGameFile(const std::string& path, GameRecord* game, std::string* error) {
    std::string text;
    if (!ReadWholeFile(path, &text)) {
        return Fail(error, "cannot read " + path);
    }
    return LoadGameText(text, game, error);
}

std::string WriteGameText(const GameRecord& game) {
    std::ostringstream out;
    out << "(game\n";
    WriteMapForm(game.map, out, "  ");
    out << "\n  (moves";
    for (const Move& move : game.moves) {
        out << "\n    " << gsexp::quote_string(MoveText(game.map, move));
    }
    out << ")";
    if (!game.evals.empty()) {
        out << "\n  (evals";
        for (int eval : game.evals) {
            out << " " << eval;
        }
        out << ")";
    }
    if (!game.analysis.empty()) {
        out << "\n  (analysis";
        for (const TurnAnalysis& turn : game.analysis) {
            out << "\n    (turn " << turn.ply << " " << turn.score << " " << turn.loss << " "
                << TurnMarkName(turn.mark) << " "
                << gsexp::quote_string(TurnMovesText(game.map, turn.best)) << ")";
        }
        out << ")";
    }
    out << ")\n";
    return out.str();
}

} // namespace shoal
