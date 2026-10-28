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
                "  shoal random <map> [games] [seed]  random playouts: results and speed\n");
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
    std::fprintf(stderr, "unknown command %s\n", command.c_str());
    PrintUsage();
    return 1;
}
