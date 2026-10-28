#pragma once

namespace shoal {

// shoal subcommands; each takes the arguments after its name
int RunShow(int argc, char** argv);
int RunMoves(int argc, char** argv);
int RunPerft(int argc, char** argv);
int RunRandom(int argc, char** argv);

} // namespace shoal
