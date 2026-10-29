#include "analysis_view.hpp"

#include "live_analysis.hpp"
#include "palette.hpp"
#include "replay.hpp"
#include "search/eval.hpp"

namespace shoal {

const TurnAnalysis* AnalysisAt(const State& state, int ply) {
    for (const TurnAnalysis& turn : state.game.analysis) {
        if (turn.ply == ply) {
            return &turn;
        }
    }
    return nullptr;
}

const TurnAnalysis* TurnAnalysisFor(const State& state, int ply) {
    return AnalysisAt(state, TurnStartPly(state, ply));
}

bool HasAnalysis(const State& state) {
    return !state.game.analysis.empty();
}

int AnalysisForPlayer0(const State& state, const TurnAnalysis& turn) {
    const Position& pos = state.positions[static_cast<std::size_t>(turn.ply)];
    return pos.side == 0 ? turn.score : -turn.score;
}

bool ShownEvalForPlayer0(const State& state, int ply, int* eval) {
    const Position& pos = state.positions[static_cast<std::size_t>(ply)];
    if (pos.result != Result::Ongoing) {
        *eval = ResultScore(pos.result, 0);
        return true;
    }
    if (ply == state.ply) {
        if (const LiveResult* live = LiveResultHere(state)) {
            *eval = live->side == 0 ? live->score : -live->score;
            return true;
        }
    }
    if (HasAnalysis(state)) {
        const TurnAnalysis* turn = TurnAnalysisFor(state, ply);
        if (turn == nullptr) {
            return false;
        }
        *eval = AnalysisForPlayer0(state, *turn);
        return true;
    }
    return EvalForPlayer0(state, ply, eval);
}

const std::vector<Move>* ShownBestTurn(const State& state) {
    if (!state.show_best) {
        return nullptr;
    }
    if (const LiveResult* live = LiveResultHere(state)) {
        return &live->best;
    }
    const TurnAnalysis* turn = AnalysisAt(state, state.ply);
    return turn != nullptr ? &turn->best : nullptr;
}

void TrimAnalysis(State* state, int ply) {
    std::vector<TurnAnalysis>& analysis = state->game.analysis;
    int start = TurnStartPly(*state, ply);
    for (std::size_t i = 0; i < analysis.size();) {
        TurnAnalysis& turn = analysis[i];
        if (turn.ply > ply) {
            analysis.erase(analysis.begin() + static_cast<std::ptrdiff_t>(i));
            continue;
        }
        if (turn.ply == start) {
            turn.loss = 0;
            turn.mark = TurnMark::None;
        }
        i++;
    }
}

Color MarkColor(TurnMark mark) {
    switch (mark) {
    case TurnMark::None:
        return kTextDim;
    case TurnMark::Inaccuracy:
        return kInaccuracy;
    case TurnMark::Mistake:
        return kMistake;
    case TurnMark::Blunder:
        return kBlunder;
    }
    return kTextDim;
}

} // namespace shoal
