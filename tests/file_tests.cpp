#include "game/position.hpp"
#include "io/map_file.hpp"
#include "tests.hpp"

#include <memory>
#include <string>

using namespace shoal;

void RunFileTests(TestResults& r) {
    auto map = std::make_unique<Map>();
    std::string error;
    bool loaded = LoadMapFile(std::string(SHOAL_SOURCE_DIR) + "/maps/tiny/hq5.sexp", map.get(),
                              &error);
    CHECK(r, loaded);
    if (!loaded) {
        std::printf("%s\n", error.c_str());
        return;
    }
    CHECK(r, map->width == 5 && map->height == 5);
    CHECK(r, map->property_count == 3);

    // writing and reading back gives the same text
    std::string text = WriteMapText(*map);
    auto again = std::make_unique<Map>();
    CHECK(r, LoadMapText(text, again.get(), &error));
    CHECK(r, WriteMapText(*again) == text);

    // a game record replays its moves and survives a round trip
    auto game = std::make_unique<GameRecord>();
    game->map = *map;
    Position pos;
    SetupStart(game->map, &pos);
    auto list = std::make_unique<MoveList>();
    for (int i = 0; i < 12 && pos.result == Result::Ongoing; i++) {
        GenerateMoves(game->map, pos, list.get());
        Move move = list->moves[i % list->count];
        game->moves.push_back(move);
        ApplyMove(game->map, &pos, move);
    }
    std::string game_text = WriteGameText(*game);
    auto read = std::make_unique<GameRecord>();
    CHECK(r, LoadGameText(game_text, read.get(), &error));
    CHECK(r, read->moves.size() == game->moves.size());
    CHECK(r, WriteGameText(*read) == game_text);
}
