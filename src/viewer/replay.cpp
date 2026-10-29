#include "replay.hpp"

#include "analysis_view.hpp"

#include <filesystem>
#include <queue>

namespace shoal {

namespace {

void BuildRows(State* state) {
    state->rows.clear();
    const std::vector<Move>& moves = state->game.moves;
    for (int i = 0; i < static_cast<int>(moves.size()); i++) {
        const Position& before = state->positions[static_cast<std::size_t>(i)];
        if (i == 0 || moves[static_cast<std::size_t>(i - 1)].kind == MoveKind::EndTurn) {
            MoveRow heading;
            heading.heading = true;
            heading.day = before.day;
            heading.side = before.side;
            state->rows.push_back(heading);
        }
        MoveRow row;
        row.day = before.day;
        row.side = before.side;
        row.move_index = i;
        state->rows.push_back(row);
    }
}

MoveReport MakeReport(const Map& map, const Position& before, const Position& after,
                      const Move& move) {
    MoveReport report;
    report.valid = true;
    report.move = move;
    report.side = before.side;
    if (move.kind == MoveKind::EndTurn) {
        return report;
    }
    if (move.kind == MoveKind::Build) {
        report.mover = move.build;
        return report;
    }
    const Unit& mover = before.units[before.unit_at[move.from]];
    report.mover = mover.type;
    report.mover_hp_before = DisplayHp(mover.hp);
    report.path = MovePath(map, before, move);
    std::uint8_t mover_after = after.unit_at[move.to];
    report.mover_hp_after = mover_after == kNoUnit ? 0 : DisplayHp(after.units[mover_after].hp);
    if (move.kind == MoveKind::Attack) {
        const Unit& target = before.units[before.unit_at[move.target]];
        report.attack = true;
        report.target = target.type;
        report.target_hp_before = DisplayHp(target.hp);
        std::uint8_t target_after = after.unit_at[move.target];
        report.target_hp_after =
            target_after == kNoUnit ? 0 : DisplayHp(after.units[target_after].hp);
    }
    if (move.kind == MoveKind::Capture) {
        int prop = map.property_at[move.to];
        report.capture = true;
        report.capture_left = after.prop_owner[prop] == before.side ? 0 : after.capture[prop];
    }
    return report;
}

bool IsTurnStart(const State& state, int ply) {
    return ply == 0 ||
           state.game.moves[static_cast<std::size_t>(ply - 1)].kind == MoveKind::EndTurn;
}

} // namespace

bool LoadRecord(State* state, const std::string& path, std::string* error) {
    std::string text;
    if (!ReadWholeFile(path, &text)) {
        *error = "cannot read " + path;
        return false;
    }
    state->path = path;
    state->game = GameRecord{};
    state->is_game_file = text.find("(game") != std::string::npos;
    bool ok = state->is_game_file ? LoadGameText(text, &state->game, error)
                                  : LoadMapText(text, &state->game.map, error);
    if (!ok) {
        return false;
    }
    RebuildPositions(state);
    return true;
}

void RebuildPositions(State* state) {
    const Map& map = state->game.map;
    state->positions.clear();
    Position pos;
    SetupStart(map, &pos);
    state->positions.push_back(pos);
    for (const Move& move : state->game.moves) {
        ApplyMove(map, &pos, move);
        state->positions.push_back(pos);
    }
    BuildRows(state);
}

int LastPly(const State& state) {
    return static_cast<int>(state.game.moves.size());
}

void SetPly(State* state, int ply) {
    if (ply < 0) {
        ply = 0;
    }
    if (ply > LastPly(*state)) {
        ply = LastPly(*state);
    }
    state->ply = ply;
    state->play = PlayState{};
    state->report = MoveReport{};
    if (ply > 0) {
        std::size_t i = static_cast<std::size_t>(ply);
        state->report = MakeReport(state->game.map, state->positions[i - 1], state->positions[i],
                                   state->game.moves[i - 1]);
    }
}

int TurnStartPly(const State& state, int ply) {
    while (ply > 0 && !IsTurnStart(state, ply)) {
        ply--;
    }
    return ply;
}

int NextTurnPly(const State& state, int ply) {
    int last = LastPly(state);
    ply++;
    while (ply < last && !IsTurnStart(state, ply)) {
        ply++;
    }
    return ply > last ? last : ply;
}

int PrevTurnPly(const State& state, int ply) {
    if (ply == 0) {
        return 0;
    }
    int start = TurnStartPly(state, ply);
    if (start < ply) {
        return start;
    }
    return TurnStartPly(state, ply - 1);
}

bool HasEvals(const State& state) {
    return !state.game.evals.empty();
}

bool EvalForPlayer0(const State& state, int ply, int* eval) {
    if (ply <= 0 || ply > static_cast<int>(state.game.evals.size())) {
        return false;
    }
    std::size_t i = static_cast<std::size_t>(ply - 1);
    int value = state.game.evals[i];
    // evals are from the view of the side that made the move
    *eval = state.positions[i].side == 0 ? value : -value;
    return true;
}

std::string SaveLine(State* state) {
    std::filesystem::path source = state->path;
    std::filesystem::path dir = source.parent_path();
    std::string stem = source.stem().string();
    std::filesystem::path out = dir / (stem + "-line.sexp");
    for (int n = 2; std::filesystem::exists(out); n++) {
        out = dir / (stem + "-line" + std::to_string(n) + ".sexp");
    }
    // evals that no longer match the moves are dropped rather than written half
    GameRecord record = state->game;
    if (record.evals.size() != record.moves.size()) {
        record.evals.clear();
    }
    if (!WriteWholeFile(out.string(), WriteGameText(record))) {
        return "";
    }
    return out.string();
}

void PlayMove(State* state, const Move& move) {
    std::size_t ply = static_cast<std::size_t>(state->ply);
    if (state->edited_from < 0 || state->edited_from > state->ply) {
        state->edited_from = state->ply;
    }
    TrimAnalysis(state, state->ply);
    state->game.moves.resize(ply);
    if (state->game.evals.size() > ply) {
        state->game.evals.resize(ply);
    }
    state->game.moves.push_back(move);
    RebuildPositions(state);
    state->scroll_to_ply = true;
    SetPly(state, state->ply + 1);
}

std::vector<int> MovePath(const Map& map, const Position& before, const Move& move) {
    std::vector<int> path;
    if (move.kind == MoveKind::EndTurn || move.kind == MoveKind::Build) {
        return path;
    }
    if (move.from == move.to) {
        path.push_back(move.from);
        return path;
    }
    const Unit& unit = before.units[before.unit_at[move.from]];
    MoveType move_type = Spec(unit.type).move_type;
    int tiles = map.width * map.height;

    // dijkstra over (tile, arrival direction); cost is move points, ties go to fewer turns
    constexpr int kTurnCost = 1;
    constexpr int kPointCost = 64;
    constexpr int kUnreached = 1 << 30;
    std::vector<int> best(static_cast<std::size_t>(tiles * 4), kUnreached);
    std::vector<int> came(static_cast<std::size_t>(tiles * 4), -1);
    using Entry = std::pair<int, int>; // cost, state
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
    for (int dir = 0; dir < 4; dir++) {
        int s = move.from * 4 + dir;
        best[static_cast<std::size_t>(s)] = 0;
        open.push({0, s});
    }
    const int dx[4] = {0, 1, 0, -1};
    const int dy[4] = {-1, 0, 1, 0};
    int goal = -1;
    while (!open.empty()) {
        auto [cost, s] = open.top();
        open.pop();
        if (cost != best[static_cast<std::size_t>(s)]) {
            continue;
        }
        int tile = s / 4;
        int dir_in = s % 4;
        if (tile == move.to) {
            goal = s;
            break;
        }
        for (int dir = 0; dir < 4; dir++) {
            int x = TileX(map, tile) + dx[dir];
            int y = TileY(map, tile) + dy[dir];
            if (x < 0 || y < 0 || x >= map.width || y >= map.height) {
                continue;
            }
            int next = TileAt(map, x, y);
            int points = MoveCost(map.tiles[next], move_type);
            if (points == 0) {
                continue;
            }
            std::uint8_t occupant = before.unit_at[next];
            if (occupant != kNoUnit && before.units[occupant].owner != unit.owner) {
                continue;
            }
            int step = points * kPointCost + (dir != dir_in && tile != move.from ? kTurnCost : 0);
            int ns = next * 4 + dir;
            if (cost + step < best[static_cast<std::size_t>(ns)]) {
                best[static_cast<std::size_t>(ns)] = cost + step;
                came[static_cast<std::size_t>(ns)] = s;
                open.push({cost + step, ns});
            }
        }
    }
    if (goal < 0) {
        path.push_back(move.from);
        path.push_back(move.to);
        return path;
    }
    // walk back from the goal; start states have no parent
    for (int s = goal; s >= 0; s = came[static_cast<std::size_t>(s)]) {
        path.push_back(s / 4);
    }
    return std::vector<int>(path.rbegin(), path.rend());
}

} // namespace shoal
