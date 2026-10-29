#pragma once

#include "game/position.hpp"
#include "search/planner.hpp"
#include "search/search.hpp"
#include "tools/book.hpp"

#include <memory>
#include <string>
#include <vector>

namespace shoal {

enum class EngineKind {
    Ply,    // alpha-beta over single unit actions
    Plan,   // turn planner
    Greedy, // the planner's greedy turn, no lookahead
    Random,
};

// "plan:t=0.5:turns=3:beam=6", "ply:t=0.2", "greedy", "random"
struct EngineSpec {
    EngineKind kind = EngineKind::Plan;
    double seconds = 0.5; // per turn
    int turns = 16;
    int beam = 12;
    int threads = 1;
    int leaf = 1;
    int narrow = 4;
    PlanWeights weights;
    std::string book_path; // a book from `shoal book`, played where it knows the position
    std::string text;
};

bool ParseEngine(const std::string& text, EngineSpec* spec);

struct Engine {
    EngineSpec spec;
    std::unique_ptr<Searcher> searcher;
    std::unique_ptr<Planner> planner;
    std::unique_ptr<Book> book;
    std::uint64_t rng = 0x2545f4914f6cdd1dull;
};

void EngineInit(Engine* engine, const EngineSpec& spec, const Map& map, std::uint64_t seed);
// one whole turn for the side to move, ending with the end move unless the game ends first
std::vector<Move> PlayTurn(Engine* engine, const Map& map, const Position& pos, int* score);

} // namespace shoal
