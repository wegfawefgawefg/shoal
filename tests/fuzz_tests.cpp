#include "game/position.hpp"
#include "io/map_file.hpp"
#include "io/map_gen.hpp"
#include "search/plan_eval.hpp"
#include "tests.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace shoal;

namespace {

std::uint64_t Next(std::uint64_t* state) {
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

// the board agrees with itself: tile index, unit list, hash, hit points, terrain
bool Consistent(const Map& map, const Position& pos, std::string* why) {
    int tile_count = map.width * map.height;
    int seen = 0;
    for (int tile = 0; tile < tile_count; tile++) {
        std::uint8_t index = pos.unit_at[tile];
        if (index == kNoUnit) {
            continue;
        }
        seen++;
        if (index >= pos.unit_count || pos.units[index].tile != tile) {
            *why = "unit_at points at the wrong unit on " + TileName(map, tile);
            return false;
        }
    }
    if (seen != pos.unit_count) {
        *why = "unit_at and the unit list disagree";
        return false;
    }
    for (int i = 0; i < pos.unit_count; i++) {
        const Unit& unit = pos.units[i];
        if (unit.hp < 1 || unit.hp > 100 || unit.ammo > Spec(unit.type).ammo) {
            *why = "unit out of range on " + TileName(map, unit.tile);
            return false;
        }
        if (MoveCost(map.tiles[unit.tile], Spec(unit.type).move_type) == 0) {
            *why = "unit on impassable terrain at " + TileName(map, unit.tile);
            return false;
        }
    }
    if (pos.hash != ComputeHash(map, pos)) {
        *why = "stale hash";
        return false;
    }
    if (pos.funds[0] < 0 || pos.funds[1] < 0) {
        *why = "negative funds";
        return false;
    }
    return true;
}

void FuzzMap(TestResults& r, const Map& map, int games, std::uint64_t seed) {
    auto list = std::make_unique<MoveList>();
    auto distances = std::make_unique<DistanceTable>();
    BuildDistances(map, distances.get());
    auto ctx = std::make_unique<PlanContext>();
    std::uint64_t rng = seed * 0x9e3779b97f4a7c15ull + 1;
    for (int g = 0; g < games; g++) {
        Position pos;
        SetupStart(map, &pos);
        PlanBegin(ctx.get(), map, *distances, pos);
        int last_side = pos.side;
        for (int ply = 0; ply < 3000 && pos.result == Result::Ongoing; ply++) {
            GenerateMoves(map, pos, list.get());
            if (list->count == 0) {
                r.failed++;
                std::printf("FAIL %s: no moves while ongoing\n", map.name.c_str());
                return;
            }
            Move move = list->moves[Next(&rng) % static_cast<std::uint64_t>(list->count)];
            // a quiet move's delta score must match the full score
            if (move.kind == MoveKind::Wait) {
                int from_prop = map.property_at[move.from];
                bool half_capture =
                    from_prop != kNoProperty && pos.capture[from_prop] != kCapturePoints;
                if (!half_capture) {
                    const Unit& unit = pos.units[pos.unit_at[move.from]];
                    Unit moved = unit;
                    moved.tile = move.to;
                    moved.done = 1;
                    int delta = PlanScore(*ctx, pos) - PlanUnitTerms(*ctx, pos, unit) +
                                PlanUnitTerms(*ctx, pos, moved);
                    Position child = pos;
                    ApplyMove(map, &child, move);
                    if (delta != PlanScore(*ctx, child)) {
                        r.failed++;
                        std::printf("FAIL %s: delta score %d != full %d for %s\n",
                                    map.name.c_str(), delta, PlanScore(*ctx, child),
                                    MoveText(map, move).c_str());
                        return;
                    }
                }
            }
            ApplyMove(map, &pos, move);
            if (pos.side != last_side) {
                PlanBegin(ctx.get(), map, *distances, pos);
                last_side = pos.side;
            }
            std::string why;
            if (!Consistent(map, pos, &why)) {
                r.failed++;
                std::printf("FAIL %s game %d ply %d after %s: %s\n", map.name.c_str(), g, ply,
                            MoveText(map, move).c_str(), why.c_str());
                return;
            }
        }
    }
    r.passed++;
}

} // namespace

void RunFuzzTests(TestResults& r) {
    std::vector<std::string> paths;
    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(std::string(SHOAL_SOURCE_DIR) + "/maps")) {
        if (entry.path().extension() == ".sexp") {
            paths.push_back(entry.path().string());
        }
    }
    for (const std::string& path : paths) {
        auto map = std::make_unique<Map>();
        std::string error;
        if (!LoadMapFile(path, map.get(), &error)) {
            r.failed++;
            std::printf("FAIL load %s: %s\n", path.c_str(), error.c_str());
            continue;
        }
        FuzzMap(r, *map, 20, 1);
    }
    for (int seed = 1; seed <= 6; seed++) {
        auto map = std::make_unique<Map>();
        MapGenOptions gen;
        gen.width = gen.height = 8 + seed * 2;
        gen.seed = static_cast<std::uint64_t>(seed);
        GenerateMap(gen, map.get());
        FuzzMap(r, *map, 4, static_cast<std::uint64_t>(seed));
    }
}
