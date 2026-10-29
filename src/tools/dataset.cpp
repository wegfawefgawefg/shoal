#include "io/map_file.hpp"
#include "tools/commands.hpp"

#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace shoal {

namespace {

// one row per turn start, from the view of the side to move: its material per unit type minus
// the enemy's (in thousands of funds), property and funds differences, captures under way,
// the day, days left, and how the game ended for that side (1 win, 0.5 draw, 0 loss)
void WriteHeader(std::FILE* out) {
    std::fprintf(out, "game,day,days_left");
    for (int type = 0; type < kUnitTypeCount; type++) {
        std::fprintf(out, ",mat_%s",
                     std::string(Spec(static_cast<UnitType>(type)).short_name).c_str());
    }
    std::fprintf(out, ",props,funds,capturing,result\n");
}

void WriteRow(std::FILE* out, int game, const Map& map, const Position& pos, Result result) {
    int side = pos.side;
    double material[kUnitTypeCount] = {};
    for (int i = 0; i < pos.unit_count; i++) {
        const Unit& unit = pos.units[i];
        double sign = unit.owner == side ? 1.0 : -1.0;
        material[static_cast<int>(unit.type)] += sign * Spec(unit.type).cost * unit.hp / 100000.0;
        for (int c = 0; c < unit.cargo_count; c++) {
            const Cargo& cargo = unit.cargo[c];
            material[static_cast<int>(cargo.type)] +=
                sign * Spec(cargo.type).cost * cargo.hp / 100000.0;
        }
    }
    int props = PropertyCount(pos, map, side) - PropertyCount(pos, map, side ^ 1);
    double funds = (pos.funds[side] - pos.funds[side ^ 1]) / 1000.0;
    double capturing = 0;
    for (int prop = 0; prop < map.property_count; prop++) {
        if (pos.capture[prop] == kCapturePoints) {
            continue;
        }
        std::uint8_t index = pos.unit_at[map.property_tiles[prop]];
        if (index == kNoUnit) {
            continue;
        }
        double done = (kCapturePoints - pos.capture[prop]) / static_cast<double>(kCapturePoints);
        capturing += pos.units[index].owner == side ? done : -done;
    }
    double outcome = 0.5;
    if (result == Result::Win0 || result == Result::Win1) {
        bool won = (result == Result::Win0) == (side == 0);
        outcome = won ? 1.0 : 0.0;
    }
    std::fprintf(out, "%d,%d,%d", game, pos.day, map.day_limit - pos.day);
    for (double value : material) {
        std::fprintf(out, ",%.3f", value);
    }
    std::fprintf(out, ",%d,%.1f,%.2f,%.1f\n", props, funds, capturing, outcome);
}

} // namespace

int RunDataset(int argc, char** argv) {
    std::vector<std::string> paths;
    std::string out_path;
    for (int i = 0; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--out" && i + 1 < argc) {
            out_path = argv[++i];
            continue;
        }
        if (std::filesystem::is_directory(arg)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(arg)) {
                if (entry.path().extension() == ".sexp") {
                    paths.push_back(entry.path().string());
                }
            }
        } else {
            paths.push_back(arg);
        }
    }
    if (paths.empty() || out_path.empty()) {
        std::fprintf(stderr, "usage: shoal dataset <games or dirs...> --out rows.csv\n");
        return 1;
    }
    std::FILE* out = std::fopen(out_path.c_str(), "w");
    if (out == nullptr) {
        std::fprintf(stderr, "cannot write %s\n", out_path.c_str());
        return 1;
    }
    WriteHeader(out);
    int games = 0;
    int rows = 0;
    for (const std::string& path : paths) {
        auto game = std::make_unique<GameRecord>();
        std::string error;
        if (!LoadGameFile(path, game.get(), &error)) {
            continue; // maps and broken files are skipped
        }
        // replay once for the result, then again writing a row at every turn start
        Position pos;
        SetupStart(game->map, &pos);
        for (const Move& move : game->moves) {
            ApplyMove(game->map, &pos, move);
        }
        Result result = pos.result;
        if (result == Result::Ongoing) {
            continue;
        }
        SetupStart(game->map, &pos);
        WriteRow(out, games, game->map, pos, result);
        rows++;
        for (const Move& move : game->moves) {
            ApplyMove(game->map, &pos, move);
            if (move.kind == MoveKind::EndTurn && pos.result == Result::Ongoing) {
                WriteRow(out, games, game->map, pos, result);
                rows++;
            }
        }
        games++;
    }
    std::fclose(out);
    std::printf("%d games, %d rows -> %s\n", games, rows, out_path.c_str());
    return 0;
}

} // namespace shoal
