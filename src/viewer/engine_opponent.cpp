#include "engine_opponent.hpp"

#include "play_mode.hpp"
#include "replay.hpp"
#include "search/planner.hpp"

#include <algorithm>
#include <atomic>
#include <thread>

namespace shoal {

// a search on copies of the map and position, so the viewer stays free while it runs
struct OpponentJob {
    int ply = 0;
    std::uint64_t hash = 0;
    Map map;
    Position pos;
    double seconds = 0;
    TurnPlan turn;
    std::atomic<bool> done{false};
    std::thread thread;
};

namespace {

const Position& Newest(const State& state) {
    return state.positions.back();
}

void RunJob(OpponentJob* job) {
    auto planner = std::make_unique<Planner>();
    PlannerInit(planner.get(), job->map);
    PlannerLimits limits;
    limits.seconds = job->seconds;
    limits.quiet = true;
    limits.threads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()) / 2);
    job->turn = PlanTurn(planner.get(), job->pos, limits).turn;
    job->done = true;
}

void StartJob(State* state) {
    auto job = std::make_shared<OpponentJob>();
    job->ply = LastPly(*state);
    job->hash = Newest(*state).hash;
    job->map = state->game.map;
    job->pos = Newest(*state);
    job->seconds = state->opponent.seconds;
    OpponentJob* raw = job.get();
    job->thread = std::thread([raw]() { RunJob(raw); });
    state->opponent.job = std::move(job);
}

// the engine's moves go in at the end of the line, as if entered by hand
void PlayJob(State* state, const OpponentJob& job) {
    if (LastPly(*state) != job.ply || Newest(*state).hash != job.hash) {
        return; // the line changed while it thought
    }
    SetPly(state, job.ply);
    for (const Move& move : job.turn.moves) {
        PlayMove(state, move);
    }
    if (Newest(*state).result == Result::Ongoing) {
        PlayMove(state, Move{});
    }
}

} // namespace

void ToggleEngineOpponent(State* state) {
    if (state->opponent.side >= 0) {
        state->opponent.side = -1;
        return;
    }
    state->opponent.side = state->positions[static_cast<std::size_t>(state->ply)].side ^ 1;
    state->mode = Mode::Play;
}

void PollEngineOpponent(State* state) {
    EngineOpponent& opponent = state->opponent;
    if (opponent.job != nullptr && opponent.job->done) {
        if (opponent.job->thread.joinable()) {
            opponent.job->thread.join();
        }
        std::shared_ptr<OpponentJob> job = std::move(opponent.job);
        if (opponent.side >= 0) {
            PlayJob(state, *job);
        }
    }
    if (opponent.side < 0 || opponent.job != nullptr || state->mode != Mode::Play) {
        return;
    }
    const Position& newest = Newest(*state);
    if (newest.result != Result::Ongoing || newest.side != opponent.side) {
        return;
    }
    // only answer at the end of the line, so stepping back through it stays read-only
    if (state->ply != LastPly(*state)) {
        return;
    }
    StartJob(state);
}

bool EngineOpponentThinking(const State& state) {
    return state.opponent.job != nullptr && !state.opponent.job->done;
}

void WaitEngineOpponent(State* state) {
    if (state->opponent.job != nullptr && state->opponent.job->thread.joinable()) {
        state->opponent.job->thread.join();
    }
    PollEngineOpponent(state);
}

} // namespace shoal
