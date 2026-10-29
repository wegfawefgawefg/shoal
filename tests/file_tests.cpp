#include "game/position.hpp"
#include "io/map_file.hpp"
#include "tests.hpp"

#include <memory>
#include <string>

using namespace shoal;

void RunFileTests(TestResults& r) {
    auto map = std::make_unique<Map>();
    std::string error;
    bool loaded =
        LoadMapFile(std::string(SHOAL_SOURCE_DIR) + "/maps/tiny/hq5.sexp", map.get(), &error);
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
    CHECK(r, read->analysis.empty());

    // engine analysis per turn start round-trips, and its best turns must be legal
    TurnAnalysis first;
    first.ply = 0;
    first.score = -1234;
    first.loss = 4100;
    first.mark = TurnMark::Mistake;
    if (game->moves[0].kind != MoveKind::EndTurn) {
        first.best.push_back(game->moves[0]);
    }
    TurnAnalysis last;
    last.ply = static_cast<int>(game->moves.size());
    last.score = 900;
    game->analysis = {first, last};
    std::string analysed_text = WriteGameText(*game);
    CHECK(r, analysed_text.find("(turn 0 -1234 4100 mistake") != std::string::npos);
    auto analysed = std::make_unique<GameRecord>();
    CHECK(r, LoadGameText(analysed_text, analysed.get(), &error));
    CHECK(r, analysed->analysis.size() == 2);
    CHECK(r, analysed->analysis[0].best == first.best);
    CHECK(r, analysed->analysis[0].mark == TurnMark::Mistake);
    CHECK(r, analysed->analysis[1].ply == last.ply && analysed->analysis[1].best.empty());
    CHECK(r, WriteGameText(*analysed) == analysed_text);

    std::string past_end = game_text;
    past_end.insert(past_end.rfind(')'), "\n  (analysis (turn 999 0 0 none \"\"))");
    CHECK(r, !LoadGameText(past_end, analysed.get(), &error));
    std::string bad_mark = game_text;
    bad_mark.insert(bad_mark.rfind(')'), "\n  (analysis (turn 0 0 0 dubious \"\"))");
    CHECK(r, !LoadGameText(bad_mark, analysed.get(), &error));
    std::string illegal = game_text;
    illegal.insert(illegal.rfind(')'), "\n  (analysis (turn 0 0 0 none \"a1 e5\"))");
    CHECK(r, !LoadGameText(illegal, analysed.get(), &error));
}
