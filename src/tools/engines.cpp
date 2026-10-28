#include "tools/engines.hpp"

#include <cstdlib>

namespace shoal {

namespace {

std::vector<std::string> SplitColons(const std::string& text) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t end = text.find(':', start);
        if (end == std::string::npos) {
            end = text.size();
        }
        parts.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    return parts;
}

std::uint64_t NextRandom(std::uint64_t* state) {
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

std::vector<Move> PlyTurn(Engine* engine, const Map& map, const Position& start, int* score) {
    std::vector<Move> moves;
    Position pos = start;
    SearchLimits limits;
    limits.quiet = true;
    // the turn's budget split over the actions it will likely take
    int actors = 1;
    for (int i = 0; i < pos.unit_count; i++) {
        actors += pos.units[i].owner == pos.side;
    }
    actors += map.production ? 1 : 0;
    limits.seconds = engine->spec.seconds / actors;
    for (int step = 0; step < 100 && pos.result == Result::Ongoing; step++) {
        SearchResult result = Search(engine->searcher.get(), pos, limits);
        Move move = result.best;
        if (!result.complete || !IsLegal(map, pos, move)) {
            move = Move{};
        }
        *score = result.score;
        moves.push_back(move);
        ApplyMove(map, &pos, move);
        if (move.kind == MoveKind::EndTurn) {
            break;
        }
    }
    return moves;
}

std::vector<Move> RandomTurn(Engine* engine, const Map& map, const Position& start) {
    std::vector<Move> moves;
    Position pos = start;
    auto list = std::make_unique<MoveList>();
    for (int step = 0; step < 200 && pos.result == Result::Ongoing; step++) {
        GenerateMoves(map, pos, list.get());
        Move move = list->moves[NextRandom(&engine->rng) % static_cast<std::uint64_t>(list->count)];
        moves.push_back(move);
        ApplyMove(map, &pos, move);
        if (move.kind == MoveKind::EndTurn) {
            break;
        }
    }
    return moves;
}

} // namespace

bool ParseEngine(const std::string& text, EngineSpec* spec) {
    *spec = EngineSpec{};
    spec->text = text;
    std::vector<std::string> parts = SplitColons(text);
    if (parts[0] == "ply") {
        spec->kind = EngineKind::Ply;
    } else if (parts[0] == "plan") {
        spec->kind = EngineKind::Plan;
    } else if (parts[0] == "greedy") {
        spec->kind = EngineKind::Greedy;
    } else if (parts[0] == "random") {
        spec->kind = EngineKind::Random;
    } else {
        return false;
    }
    for (std::size_t i = 1; i < parts.size(); i++) {
        std::size_t eq = parts[i].find('=');
        if (eq == std::string::npos) {
            return false;
        }
        std::string key = parts[i].substr(0, eq);
        std::string value = parts[i].substr(eq + 1);
        if (key == "t") {
            spec->seconds = std::atof(value.c_str());
        } else if (key == "turns") {
            spec->turns = std::atoi(value.c_str());
        } else if (key == "beam") {
            spec->beam = std::atoi(value.c_str());
        } else {
            return false;
        }
    }
    return true;
}

void EngineInit(Engine* engine, const EngineSpec& spec, const Map& map, std::uint64_t seed) {
    engine->spec = spec;
    engine->rng ^= seed * 0x9e3779b97f4a7c15ull;
    if (spec.kind == EngineKind::Ply) {
        engine->searcher = std::make_unique<Searcher>();
        SearcherInit(engine->searcher.get(), map, 20);
    }
    if (spec.kind == EngineKind::Plan || spec.kind == EngineKind::Greedy) {
        engine->planner = std::make_unique<Planner>();
        PlannerInit(engine->planner.get(), map);
    }
}

std::vector<Move> PlayTurn(Engine* engine, const Map& map, const Position& pos, int* score) {
    *score = 0;
    switch (engine->spec.kind) {
    case EngineKind::Ply:
        return PlyTurn(engine, map, pos, score);
    case EngineKind::Random:
        return RandomTurn(engine, map, pos);
    case EngineKind::Plan:
    case EngineKind::Greedy:
        break;
    }
    TurnPlan turn;
    if (engine->spec.kind == EngineKind::Greedy) {
        turn = GreedyTurn(engine->planner.get(), pos);
        *score = turn.score;
    } else {
        PlannerLimits limits;
        limits.seconds = engine->spec.seconds;
        limits.max_turns = engine->spec.turns;
        limits.beam_width = engine->spec.beam;
        limits.quiet = true;
        PlannerResult result = PlanTurn(engine->planner.get(), pos, limits);
        turn = result.turn;
        *score = result.score;
    }
    std::vector<Move> moves = turn.moves;
    if (turn.end.result == Result::Ongoing) {
        moves.push_back(Move{});
    }
    return moves;
}

} // namespace shoal
