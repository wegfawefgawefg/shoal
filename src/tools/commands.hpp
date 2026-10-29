#pragma once

#include "game/position.hpp"
#include "io/map_file.hpp"

namespace shoal {

// a map file starts the game; a game file replays to its last move
bool LoadPosition(const char* path, GameRecord* game, Position* pos);
const char* ResultTextFor(Result result);

// shoal subcommands; each takes the arguments after its name
int RunShow(int argc, char** argv);
int RunMoves(int argc, char** argv);
int RunPerft(int argc, char** argv);
int RunRandom(int argc, char** argv);
int RunSearch(int argc, char** argv);
int RunSelfPlay(int argc, char** argv);
int RunPlan(int argc, char** argv);
int RunPlanPlay(int argc, char** argv);
int RunAnalyse(int argc, char** argv);
int RunMatch(int argc, char** argv);
int RunGenMap(int argc, char** argv);
int RunTune(int argc, char** argv);
int RunStats(int argc, char** argv);

} // namespace shoal
