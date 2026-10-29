#include "game/position.hpp"
#include "io/board_text.hpp"
#include "io/map_file.hpp"
#include "tools/commands.hpp"

#include <cstdio>
#include <cstring>
#include <string>

namespace {

void PrintUsage() {
    std::printf("shoal: an advance wars engine\n"
                "  shoal show <map|game>           print the start (or end) position\n"
                "  shoal moves <map>               list the legal first moves\n"
                "  shoal perft <map> <plies>       count move paths to a depth\n"
                "  shoal random <map> [games] [seed]  random playouts: results and speed\n"
                "  shoal search <map|game> [--depth N] [--time S] [--solve] [--table log2]\n"
                "  shoal selfplay <map> [--time S] [--depth N] [--out game.sexp]\n"
                "  shoal plan <map|game> [--time S] [--turns N] [--beam W]\n"
                "  shoal planplay <map> [--time S] [--turns N] [--beam W] [--out game.sexp]\n"
                "  shoal analyse <game> [--time S] [--threads N] [--out annotated.sexp]\n"
                "  shoal match --a ENGINE --b ENGINE [maps...] [--gen SIZE COUNT] [--threads T]\n"
                "  shoal genmap [--size N] [--seed S] [--days D] [--bases B] [--water]\n"
                "  shoal tune [--rounds N] [--maps M] [--threads T] [--time S] [--rate R]\n"
                "  shoal stats <games or dirs...>  builds, trades and results over games\n"
                "  shoal dataset <games or dirs...> --out rows.csv  turn-start features\n"
                "  shoal fuzz [--games N] [--seed S] [--threads T]  random games, board checks\n"
                "  shoal book <map> [--turns N] [--time S] [--threads T] [--branch K] --out f\n"
                "  shoal duel [stepper striker]  even fights by footing, played to the end\n");
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || std::strcmp(argv[1], "help") == 0) {
        PrintUsage();
        return argc < 2 ? 1 : 0;
    }
    std::string command = argv[1];
    if (command == "show") {
        return shoal::RunShow(argc - 2, argv + 2);
    }
    if (command == "moves") {
        return shoal::RunMoves(argc - 2, argv + 2);
    }
    if (command == "perft") {
        return shoal::RunPerft(argc - 2, argv + 2);
    }
    if (command == "random") {
        return shoal::RunRandom(argc - 2, argv + 2);
    }
    if (command == "search") {
        return shoal::RunSearch(argc - 2, argv + 2);
    }
    if (command == "selfplay") {
        return shoal::RunSelfPlay(argc - 2, argv + 2);
    }
    if (command == "plan") {
        return shoal::RunPlan(argc - 2, argv + 2);
    }
    if (command == "planplay") {
        return shoal::RunPlanPlay(argc - 2, argv + 2);
    }
    if (command == "analyse" || command == "analyze") {
        return shoal::RunAnalyse(argc - 2, argv + 2);
    }
    if (command == "match") {
        return shoal::RunMatch(argc - 2, argv + 2);
    }
    if (command == "genmap") {
        return shoal::RunGenMap(argc - 2, argv + 2);
    }
    if (command == "tune") {
        return shoal::RunTune(argc - 2, argv + 2);
    }
    if (command == "stats") {
        return shoal::RunStats(argc - 2, argv + 2);
    }
    if (command == "dataset") {
        return shoal::RunDataset(argc - 2, argv + 2);
    }
    if (command == "fuzz") {
        return shoal::RunFuzz(argc - 2, argv + 2);
    }
    if (command == "book") {
        return shoal::RunBook(argc - 2, argv + 2);
    }
    if (command == "duel") {
        return shoal::RunDuelTables(argc - 2, argv + 2);
    }
    std::fprintf(stderr, "unknown command %s\n", command.c_str());
    PrintUsage();
    return 1;
}
