#include "live_analysis.hpp"

#include "search/analysis.hpp"

#include <atomic>
#include <thread>

namespace shoal {

// the search owns copies of the map and position, so the viewer can reload or edit the game
// while it runs; the thread is joined before the job is dropped
struct LiveJob {
    int id = 0;
    int ply = 0;
    Map map;
    Position pos;
    double seconds = 0;
    PositionAnalysis analysis;
    std::atomic<bool> done{false};
    std::thread thread;
};

namespace {

const Position& Shown(const State& state) {
    return state.positions[static_cast<std::size_t>(state.ply)];
}

void RunJob(LiveJob* job) {
    auto planner = std::make_unique<Planner>();
    PlannerInit(planner.get(), job->map);
    AnalysePosition(planner.get(), job->pos, job->seconds, &job->analysis);
    job->done = true;
}

void KeepResult(State* state, const LiveJob& job) {
    LiveResult& r = state->live.result;
    r.valid = true;
    r.ply = job.ply;
    r.hash = job.pos.hash;
    r.side = job.pos.side;
    r.score = job.analysis.score;
    r.best = job.analysis.best.moves;
    r.turns = job.analysis.turns;
    r.seconds = job.analysis.seconds;
}

const LiveJob* LatestJob(const State& state) {
    for (const std::shared_ptr<LiveJob>& job : state.live.jobs) {
        if (job->id == state.live.latest_id) {
            return job.get();
        }
    }
    return nullptr;
}

bool MadeFor(const State& state, int ply, std::uint64_t hash) {
    return ply == state.ply && hash == Shown(state).hash;
}

} // namespace

void StartLiveAnalysis(State* state) {
    const Position& pos = Shown(*state);
    if (LiveThinkingHere(*state)) {
        return;
    }
    auto job = std::make_shared<LiveJob>();
    job->id = ++state->live.latest_id;
    job->ply = state->ply;
    job->map = state->game.map;
    job->pos = pos;
    job->seconds = state->live.seconds * 0.5;
    LiveJob* raw = job.get();
    job->thread = std::thread([raw]() { RunJob(raw); });
    state->live.jobs.push_back(std::move(job));
}

void PollLiveAnalysis(State* state) {
    std::vector<std::shared_ptr<LiveJob>>& jobs = state->live.jobs;
    for (std::size_t i = 0; i < jobs.size();) {
        LiveJob& job = *jobs[i];
        if (!job.done) {
            i++;
            continue;
        }
        if (job.thread.joinable()) {
            job.thread.join();
        }
        if (job.id == state->live.latest_id) {
            KeepResult(state, job);
        }
        jobs.erase(jobs.begin() + static_cast<std::ptrdiff_t>(i));
    }
}

bool LiveAnalysisRunning(const State& state) {
    return !state.live.jobs.empty();
}

bool LiveThinkingHere(const State& state) {
    const LiveJob* job = LatestJob(state);
    return job != nullptr && !job->done && MadeFor(state, job->ply, job->pos.hash);
}

const LiveResult* LiveResultHere(const State& state) {
    const LiveResult& r = state.live.result;
    if (!r.valid || !MadeFor(state, r.ply, r.hash)) {
        return nullptr;
    }
    return &r;
}

void WaitLiveAnalysis(State* state) {
    for (std::shared_ptr<LiveJob>& job : state->live.jobs) {
        if (job->thread.joinable()) {
            job->thread.join();
        }
    }
    PollLiveAnalysis(state);
}

} // namespace shoal
