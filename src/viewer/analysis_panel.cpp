// the engine box (verdict on the shown turn, best turn, live analysis) and the eval graph
#include "analysis_panel.hpp"

#include "analysis_view.hpp"
#include "layout.hpp"
#include "live_analysis.hpp"
#include "palette.hpp"
#include "replay.hpp"
#include "search/eval.hpp"
#include "side_panel.hpp"

#include <cmath>
#include <cstdio>

namespace shoal {

namespace {

constexpr int kBestLines = 3;
// eval at which the graph is about three quarters of the way to its edge, as the bar
constexpr float kGraphScale = 12000.0f;

std::string ScoreWord(int score) {
    if (score > kWinThreshold) {
        return "won";
    }
    if (score < -kWinThreshold) {
        return "lost";
    }
    return FormatEval(score);
}

Color EvalColor(int eval) {
    return eval > 0 ? kPlayer[0] : eval < 0 ? kPlayer[1] : kTextDim;
}

// "?? lost 12.7k", or "?? won > lost" when the turn changed the result
std::string Verdict(const TurnAnalysis& turn) {
    if (turn.mark == TurnMark::None) {
        return "played turn: fine";
    }
    if (turn.loss < kWinThreshold) {
        return "played turn lost " + FormatEval(turn.loss).substr(1);
    }
    // a result swing: name both ends, the played one as the judgement saw it
    auto word = [](int score) { return score == 0 ? std::string("draw") : ScoreWord(score); };
    std::string after = word(turn.score - turn.loss);
    return "played turn: " + word(turn.score) + " > " + after;
}

// the best turn as "1 e6 c4  2 f4 d3 ...", wrapped into the box, numbers in the arrow colour
void DrawBestMoves(State* state, const std::vector<Move>& best, float y) {
    const Rect& box = state->layout.engine;
    const Map& map = state->game.map;
    float size = Ui(*state, 13);
    float line = LineHeight(size);
    float gap = Ui(*state, 9);
    float right = box.x + box.w;
    if (best.empty()) {
        DrawText(&state->font, box.x, y, size, kTextDim, "best: end the turn");
        return;
    }
    float x = box.x;
    int row = 0;
    std::string ellipsis = "...";
    for (std::size_t i = 0; i < best.size(); i++) {
        std::string number = std::to_string(i + 1);
        std::string text = MoveText(map, best[i]);
        float w = TextWidth(&state->font, size, number) + Ui(*state, 4) +
                  TextWidth(&state->font, size, text);
        bool last_row = row == kBestLines - 1;
        float room = last_row && i + 1 < best.size()
                         ? right - TextWidth(&state->font, size, ellipsis) - gap
                         : right;
        if (x > box.x && x + w > room) {
            if (last_row) {
                DrawText(&state->font, x, y, size, kTextDim, ellipsis);
                return;
            }
            row++;
            y += line;
            x = box.x;
        }
        x += DrawText(&state->font, x, y, size, kBest, number) + Ui(*state, 4);
        x += DrawText(&state->font, x, y, size, kText, text) + gap;
    }
}

float GraphY(const Rect& graph, int eval) {
    float share = 0.5f + 0.5f * std::tanh(static_cast<float>(eval) / kGraphScale);
    if (eval > kWinThreshold) {
        share = 1;
    } else if (eval < -kWinThreshold) {
        share = 0;
    }
    float inset = 2;
    return graph.y + inset + (graph.h - 2 * inset) * (1 - share);
}

float GraphX(const State& state, int ply) {
    const Rect& graph = state.layout.graph;
    int last = LastPly(state);
    if (last == 0) {
        return graph.x;
    }
    return graph.x + graph.w * static_cast<float>(ply) / static_cast<float>(last);
}

struct GraphPoint {
    int ply = 0;
    int eval = 0; // player 0's view
    TurnMark mark = TurnMark::None;
};

// analysis turn starts when the file has them, else every move's eval
std::vector<GraphPoint> GraphPoints(const State& state) {
    std::vector<GraphPoint> points;
    if (HasAnalysis(state)) {
        for (const TurnAnalysis& turn : state.game.analysis) {
            points.push_back({turn.ply, AnalysisForPlayer0(state, turn), turn.mark});
        }
        // a game that ended mid-turn ends the graph on its result
        int last = LastPly(state);
        const Position& end = state.positions[static_cast<std::size_t>(last)];
        if (!points.empty() && points.back().ply < last && end.result != Result::Ongoing) {
            points.push_back({last, ResultScore(end.result, 0), TurnMark::None});
        }
        return points;
    }
    for (int ply = 1; ply <= LastPly(state); ply++) {
        int eval = 0;
        if (EvalForPlayer0(state, ply, &eval)) {
            points.push_back({ply, eval, TurnMark::None});
        }
    }
    return points;
}

} // namespace

void DrawEnginePanel(State* state) {
    const Rect& box = state->layout.engine;
    if (box.h <= 0) {
        return;
    }
    const Position& pos = state->positions[static_cast<std::size_t>(state->ply)];
    float size = Ui(*state, 15);
    float small = Ui(*state, 13);
    FillRect(state->renderer, Rect{box.x, box.y, box.w, 1}, kPanelLine);
    float y = box.y + Ui(*state, 8);
    float x = box.x + DrawText(&state->font, box.x, y, size, kTextFaint, "Engine") + Ui(*state, 10);
    float text_y = y + (LineHeight(size) - LineHeight(small)) * 0.5f;
    float best_y = y + LineHeight(size) + Ui(*state, 4);
    const LiveResult* live = LiveResultHere(*state);
    const TurnAnalysis* here = AnalysisAt(*state, state->ply);
    const TurnAnalysis* turn = TurnAnalysisFor(*state, state->ply);

    if (pos.result != Result::Ongoing) {
        DrawText(&state->font, x, text_y, small, kTextDim, "game over");
        return;
    }
    if (LiveThinkingHere(*state)) {
        int dots = static_cast<int>(SDL_GetTicks() / 300 % 4);
        DrawText(&state->font, x, text_y, small, kHighlight,
                 "thinking" + std::string(static_cast<std::size_t>(dots), '.'));
        return;
    }
    if (live != nullptr) {
        int eval = live->side == 0 ? live->score : -live->score;
        x += DrawText(&state->font, x, y, size, EvalColor(eval), FormatEval(eval));
        char info[64];
        std::snprintf(info, sizeof(info), "   live, %d turns deep, %.1fs", live->turns,
                      live->seconds);
        DrawText(&state->font, x, text_y, small, kTextDim, info);
        DrawBestMoves(state, live->best, best_y);
        return;
    }
    if (turn == nullptr) {
        DrawText(&state->font, x, text_y, small, kTextDim, "A  analyse this position");
        return;
    }
    int eval = AnalysisForPlayer0(*state, *turn);
    if (here == nullptr) {
        x += DrawText(&state->font, x, text_y, small, kTextDim, "turn began ");
    }
    x += DrawText(&state->font, x, y, size, EvalColor(eval), FormatEval(eval)) + Ui(*state, 10);
    if (turn->mark != TurnMark::None) {
        x += DrawText(&state->font, x, y, size, MarkColor(turn->mark), TurnMarkSymbol(turn->mark)) +
             Ui(*state, 6);
    }
    DrawText(&state->font, x, text_y, small, turn->mark == TurnMark::None ? kTextDim : kText,
             Verdict(*turn));
    if (here != nullptr) {
        DrawBestMoves(state, here->best, best_y);
    } else {
        DrawText(&state->font, box.x, best_y, small, kTextFaint, "A  analyse from here");
    }
}

void DrawEvalGraph(State* state) {
    const Rect& graph = state->layout.graph;
    if (graph.h <= 0) {
        return;
    }
    std::vector<GraphPoint> points = GraphPoints(*state);
    SDL_Renderer* r = state->renderer;
    FillRoundRect(r, graph, Ui(*state, 4), Shade(kPlayerDeep[1], 0.45f));
    if (points.empty()) {
        return;
    }

    // player 0's share under the curve, a column at a time
    int columns = static_cast<int>(graph.w);
    std::size_t at = 0;
    Color fill = Shade(kPlayer[0], 0.8f);
    for (int c = 0; c < columns; c++) {
        float px = graph.x + static_cast<float>(c) + 0.5f;
        while (at + 1 < points.size() && GraphX(*state, points[at + 1].ply) <= px) {
            at++;
        }
        // straight between points, like the line on top
        float ax = GraphX(*state, points[at].ply);
        float y = GraphY(graph, points[at].eval);
        if (at + 1 < points.size() && px > ax) {
            float bx = GraphX(*state, points[at + 1].ply);
            float t = bx > ax ? (px - ax) / (bx - ax) : 0;
            y += (GraphY(graph, points[at + 1].eval) - y) * t;
        }
        FillRect(r, Rect{graph.x + static_cast<float>(c), y, 1, graph.y + graph.h - y}, fill);
    }
    FillRect(r, Rect{graph.x, graph.y + graph.h * 0.5f, graph.w, 1}, WithAlpha(kInk, 150));
    for (std::size_t i = 0; i + 1 < points.size(); i++) {
        Vec2 a{GraphX(*state, points[i].ply), GraphY(graph, points[i].eval)};
        Vec2 b{GraphX(*state, points[i + 1].ply), GraphY(graph, points[i + 1].eval)};
        Line(r, a, b, Ui(*state, 2), kText);
    }

    // the current ply, then the marked turns on top
    float cx = std::round(GraphX(*state, state->ply));
    FillRect(r, Rect{cx - 1, graph.y, 2, graph.h}, kHighlight);
    for (const GraphPoint& point : points) {
        if (point.mark == TurnMark::None) {
            continue;
        }
        Vec2 c{GraphX(*state, point.ply), GraphY(graph, point.eval)};
        FillCircle(r, c, Ui(*state, 5), kInk);
        FillCircle(r, c, Ui(*state, 4), MarkColor(point.mark));
    }
}

int GraphPlyAt(const State& state, float x, float y) {
    const Rect& graph = state.layout.graph;
    if (graph.h <= 0 || !Contains(graph, x, y)) {
        return -1;
    }
    std::vector<GraphPoint> points = GraphPoints(state);
    if (points.empty() || !HasAnalysis(state)) {
        int last = LastPly(state);
        return static_cast<int>(std::lround((x - graph.x) / graph.w * static_cast<float>(last)));
    }
    // with analysis, snap to the nearest turn start
    int best = points[0].ply;
    float best_d = 1e9f;
    for (const GraphPoint& point : points) {
        float d = std::fabs(GraphX(state, point.ply) - x);
        if (d < best_d) {
            best_d = d;
            best = point.ply;
        }
    }
    return best;
}

} // namespace shoal
