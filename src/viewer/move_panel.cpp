#include "layout.hpp"
#include "palette.hpp"
#include "replay.hpp"
#include "side_panel.hpp"

#include <algorithm>

namespace shoal {

namespace {

float RowHeight(const State& state, const MoveRow& row) {
    return row.heading ? Ui(state, 28) : Ui(state, 23);
}

float ContentHeight(const State& state) {
    float total = 0;
    for (const MoveRow& row : state.rows) {
        total += RowHeight(state, row);
    }
    return total;
}

float RowTop(const State& state, int row) {
    float y = 0;
    for (int i = 0; i < row; i++) {
        y += RowHeight(state, state.rows[static_cast<std::size_t>(i)]);
    }
    return y;
}

// the row holding the move that led to the current ply, -1 at the start
int CurrentRow(const State& state) {
    for (int i = 0; i < static_cast<int>(state.rows.size()); i++) {
        const MoveRow& row = state.rows[static_cast<std::size_t>(i)];
        if (!row.heading && row.move_index == state.ply - 1) {
            return i;
        }
    }
    return -1;
}

void ClampScroll(State* state) {
    float most = ContentHeight(*state) - state->layout.move_list.h;
    state->list_scroll = std::clamp(state->list_scroll, 0.0f, std::max(0.0f, most));
}

void FollowPly(State* state) {
    int row = CurrentRow(*state);
    if (row < 0) {
        state->list_scroll = 0;
        return;
    }
    float top = RowTop(*state, row);
    float bottom = top + RowHeight(*state, state->rows[static_cast<std::size_t>(row)]);
    // keep the turn heading in view too when we can
    if (row > 0 && state->rows[static_cast<std::size_t>(row - 1)].heading) {
        top = RowTop(*state, row - 1);
    }
    float view = state->layout.move_list.h;
    float margin = Ui(*state, 30);
    if (top < state->list_scroll + margin) {
        state->list_scroll = top - margin;
    } else if (bottom > state->list_scroll + view - margin) {
        state->list_scroll = bottom - view + margin;
    }
}

void DrawHeading(State* state, const MoveRow& row, Rect rect, bool current_turn) {
    float size = Ui(*state, 13);
    float y = rect.y + rect.h - LineHeight(size) - Ui(*state, 3);
    FillRect(state->renderer, Rect{rect.x, rect.y + Ui(*state, 4), rect.w, 1}, kPanelLine);
    FillCircle(state->renderer, Vec2{rect.x + Ui(*state, 6), y + LineHeight(size) * 0.5f},
               Ui(*state, 4), kPlayer[row.side]);
    std::string text = "Day " + std::to_string(row.day) + "  " + kPlayerName[row.side];
    DrawText(&state->font, rect.x + Ui(*state, 16), y, size, current_turn ? kText : kTextDim, text);
}

void DrawMoveRow(State* state, int index, const MoveRow& row, Rect rect, bool current) {
    const Map& map = state->game.map;
    float size = Ui(*state, 14);
    bool hover = index == state->hover_row;
    if (current) {
        FillRoundRect(state->renderer, rect, Ui(*state, 4), kPanelRaised);
        FillRoundRect(state->renderer, Rect{rect.x, rect.y + 2, Ui(*state, 3), rect.h - 4},
                      Ui(*state, 2), kPlayer[row.side]);
    } else if (hover) {
        FillRoundRect(state->renderer, rect, Ui(*state, 4), WithAlpha(kPanelRaised, 160));
    }
    float y = rect.y + (rect.h - LineHeight(size)) * 0.5f;
    const Move& move = state->game.moves[static_cast<std::size_t>(row.move_index)];
    bool hand = state->edited_from >= 0 && row.move_index >= state->edited_from;
    std::string number = std::to_string(row.move_index + 1);
    DrawTextAligned(&state->font, rect.x + Ui(*state, 40), y, size, hand ? kHighlight : kTextFaint,
                    number, Align::Right);
    Color text_color = move.kind == MoveKind::EndTurn ? kTextDim : kText;
    DrawText(&state->font, rect.x + Ui(*state, 52), y, size, text_color, MoveText(map, move));
    int eval = 0;
    if (EvalForPlayer0(*state, row.move_index + 1, &eval)) {
        Color color = eval > 0 ? kPlayer[0] : eval < 0 ? kPlayer[1] : kTextDim;
        DrawTextAligned(&state->font, rect.x + rect.w - Ui(*state, 8), y, size, color,
                        FormatEval(eval), Align::Right);
    }
}

} // namespace

int RowPly(const State& state, int row) {
    const MoveRow& r = state.rows[static_cast<std::size_t>(row)];
    if (r.heading) {
        // the next row is the turn's first move; its ply-before is where the turn starts
        const MoveRow& next = state.rows[static_cast<std::size_t>(row + 1)];
        return next.move_index;
    }
    return r.move_index + 1;
}

int MoveRowAt(const State& state, float x, float y) {
    const Rect& list = state.layout.move_list;
    if (!Contains(list, x, y)) {
        return -1;
    }
    float local = y - list.y + state.list_scroll;
    float top = 0;
    for (int i = 0; i < static_cast<int>(state.rows.size()); i++) {
        float h = RowHeight(state, state.rows[static_cast<std::size_t>(i)]);
        if (local >= top && local < top + h) {
            return i;
        }
        top += h;
    }
    return -1;
}

void ScrollMoveList(State* state, float amount) {
    state->list_scroll += amount;
    state->scroll_to_ply = false;
    ClampScroll(state);
}

void DrawMoveList(State* state) {
    const Rect& list = state->layout.move_list;
    if (state->rows.empty()) {
        float size = Ui(*state, 14);
        std::string text = state->mode == Mode::Play ? "Click a unit to move it."
                                                     : "No moves yet. Press P to play.";
        DrawText(&state->font, list.x, list.y + Ui(*state, 6), size, kTextDim, text);
        return;
    }
    if (state->scroll_to_ply) {
        FollowPly(state);
        state->scroll_to_ply = false;
    }
    ClampScroll(state);

    SDL_Rect clip{static_cast<int>(list.x) - 2, static_cast<int>(list.y),
                  static_cast<int>(list.w) + 4, static_cast<int>(list.h)};
    SDL_SetRenderClipRect(state->renderer, &clip);
    int current = CurrentRow(*state);
    int turn_start = TurnStartPly(*state, state->ply > 0 ? state->ply - 1 : 0);
    float y = list.y - state->list_scroll;
    for (int i = 0; i < static_cast<int>(state->rows.size()); i++) {
        const MoveRow& row = state->rows[static_cast<std::size_t>(i)];
        float h = RowHeight(*state, row);
        if (y + h >= list.y && y <= list.y + list.h) {
            Rect rect{list.x, y, list.w, h};
            if (row.heading) {
                bool current_turn =
                    state->rows[static_cast<std::size_t>(i + 1)].move_index == turn_start;
                DrawHeading(state, row, rect, current_turn && state->ply > 0);
            } else {
                DrawMoveRow(state, i, row, rect, i == current);
            }
        }
        y += h;
    }
    SDL_SetRenderClipRect(state->renderer, nullptr);

    // a thin scroll thumb when the list overflows
    float content = ContentHeight(*state);
    if (content > list.h) {
        float thumb_h = std::max(Ui(*state, 24), list.h * list.h / content);
        float thumb_y = list.y + (list.h - thumb_h) * state->list_scroll / (content - list.h);
        FillRoundRect(state->renderer,
                      Rect{list.x + list.w + Ui(*state, 6), thumb_y, Ui(*state, 4), thumb_h},
                      Ui(*state, 2), kPanelLine);
    }
}

} // namespace shoal
