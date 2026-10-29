#include "side_panel.hpp"

#include "analysis_panel.hpp"
#include "analysis_view.hpp"
#include "layout.hpp"
#include "palette.hpp"
#include "replay.hpp"
#include "search/eval.hpp"

#include <cmath>
#include <cstdlib>

namespace shoal {

namespace {

// eval at which the bar is about three quarters full
constexpr float kEvalBarScale = 12000.0f;

int ArmyValue(const Position& pos, int owner) {
    int value = 0;
    for (int i = 0; i < pos.unit_count; i++) {
        const Unit& unit = pos.units[i];
        if (unit.owner != owner) {
            continue;
        }
        value += Spec(unit.type).cost * DisplayHp(unit.hp) / 10;
        for (int c = 0; c < unit.cargo_count; c++) {
            value += Spec(unit.cargo[c].type).cost * DisplayHp(unit.cargo[c].hp) / 10;
        }
    }
    return value;
}

std::string Thousands(int value) {
    if (value >= 100000) {
        return std::to_string(value / 1000) + "k";
    }
    if (value >= 1000) {
        int tenths = (value + 50) / 100;
        return std::to_string(tenths / 10) + "." + std::to_string(tenths % 10) + "k";
    }
    return std::to_string(value);
}

void DrawPlayerCard(State* state, Rect card, int player, const Position& pos) {
    const Map& map = state->game.map;
    bool to_move = pos.side == player && pos.result == Result::Ongoing;
    FillRoundRect(state->renderer, card, Ui(*state, 6), to_move ? kPanelRaised : kPanel);
    FillRoundRect(state->renderer, Rect{card.x, card.y, Ui(*state, 5), card.h}, Ui(*state, 3),
                  to_move ? kPlayer[player] : kPlayerDeep[player]);
    float x = card.x + Ui(*state, 14);
    float name_size = Ui(*state, 17);
    float info_size = Ui(*state, 13);
    float y = card.y + Ui(*state, 5);
    DrawText(&state->font, x, y, name_size, kPlayer[player], kPlayerName[player]);
    float name_w = TextWidth(&state->font, name_size, kPlayerName[player]);
    std::string who = "player " + std::to_string(player) + (to_move ? "  to move" : "");
    DrawText(&state->font, x + name_w + Ui(*state, 8), y + Ui(*state, 3), info_size,
             to_move ? kText : kTextFaint, who);
    std::string funds = FormatFunds(pos.funds[player]);
    DrawTextAligned(&state->font, card.x + card.w - Ui(*state, 10), y, name_size, kText, funds,
                    Align::Right);
    int props = PropertyCount(pos, map, player);
    int units = UnitCount(pos, player);
    for (int i = 0; i < pos.unit_count; i++) {
        units += pos.units[i].owner == player ? pos.units[i].cargo_count : 0;
    }
    std::string info = std::to_string(props) + (props == 1 ? " property   " : " properties   ") +
                       std::to_string(units) + (units == 1 ? " unit" : " units") + "   army " +
                       Thousands(ArmyValue(pos, player));
    DrawText(&state->font, x, y + LineHeight(name_size), info_size, kTextDim, info);
}

// who won and how: the hq, a rout, or the day limit
std::string ResultLine(const State& state, const Position& pos) {
    if (pos.result == Result::Draw) {
        return "Draw on the day limit";
    }
    int winner = pos.result == Result::Win0 ? 0 : 1;
    std::string line = std::string(kPlayerName[winner]) + " wins";
    const MoveReport& r = state.report;
    if (r.capture && r.capture_left == 0 && state.game.map.tiles[r.move.to] == Terrain::Hq) {
        return line + ", HQ taken";
    }
    if (UnitCount(pos, 1 - winner) == 0) {
        return line + ", rout";
    }
    return line + " on the day limit";
}

void DrawHeader(State* state) {
    const Layout& l = state->layout;
    const Map& map = state->game.map;
    const Position& pos = state->positions[static_cast<std::size_t>(state->ply)];
    float title = Ui(*state, 22);
    float small = Ui(*state, 14);
    float y = l.header.y;
    DrawText(&state->font, l.header.x, y, title, kText, map.name.empty() ? "untitled" : map.name);
    std::string day = "day " + std::to_string(pos.day) + " of " + std::to_string(map.day_limit);
    DrawTextAligned(&state->font, l.header.x + l.header.w, y + Ui(*state, 4), small, kTextDim, day,
                    Align::Right);
    y += LineHeight(title);

    // status: who moves, or a pill with the result
    if (pos.result != Result::Ongoing) {
        std::string text = ResultLine(*state, pos);
        Color fill = pos.result == Result::Win0   ? kPlayer[0]
                     : pos.result == Result::Win1 ? kPlayer[1]
                                                  : kText;
        float w = TextWidth(&state->font, small, text) + Ui(*state, 16);
        FillRoundRect(state->renderer, Rect{l.header.x, y, w, LineHeight(small)},
                      LineHeight(small) * 0.5f, fill);
        DrawText(&state->font, l.header.x + Ui(*state, 8), y, small, kInk, text);
    } else {
        std::string status = std::string(kPlayerName[pos.side]) + " to move";
        if (state->mode == Mode::Play) {
            status += "   play mode";
        }
        if (state->opponent.side >= 0) {
            status += "   engine plays " + std::string(kPlayerName[state->opponent.side]);
            if (state->opponent.job != nullptr) {
                status += ", thinking...";
            }
        }
        DrawText(&state->font, l.header.x, y, small, kTextDim, status);
    }
    int eval = 0;
    if (ShownEvalForPlayer0(*state, state->ply, &eval)) {
        DrawTextAligned(&state->font, l.header.x + l.header.w, y, small, kText,
                        "eval " + FormatEval(eval), Align::Right);
    }
    y += LineHeight(small) + Ui(*state, 8);

    float card_h = Ui(*state, 48);
    for (int player = 0; player < 2; player++) {
        DrawPlayerCard(state, Rect{l.header.x, y, l.header.w, card_h}, player, pos);
        y += card_h + Ui(*state, 6);
    }
}

std::string UnitName(UnitType type) {
    return std::string(Spec(type).name);
}

int FocusTile(const State& state, const Position& pos);
std::string UnitLine(const Map& map, const Unit& unit);

// one line on the focused unit, amber when its tank is low
void DrawFocusLine(State* state, float y) {
    const Position& pos = state->positions[static_cast<std::size_t>(state->ply)];
    int tile = FocusTile(*state, pos);
    if (tile < 0) {
        return;
    }
    const Unit& unit = pos.units[pos.unit_at[tile]];
    Color color = LowFuel(unit) ? Color{0xf4, 0xb7, 0x3c} : kTextFaint;
    DrawText(&state->font, state->layout.report.x, y, Ui(*state, 13), color,
             UnitLine(state->game.map, unit));
}

// the unit under the cursor, else the one selected in play mode, else the one that just moved
int FocusTile(const State& state, const Position& pos) {
    for (int tile : {state.hover_tile, state.play.selected}) {
        if (tile >= 0 && pos.unit_at[tile] != kNoUnit) {
            return tile;
        }
    }
    const MoveReport& r = state.report;
    if (r.valid && r.move.kind != MoveKind::EndTurn && pos.unit_at[r.move.to] != kNoUnit) {
        return r.move.to;
    }
    return -1;
}

// "Fighter b4: 7 hp, fuel 22/99, ammo 5/9"
std::string UnitLine(const Map& map, const Unit& unit) {
    const UnitSpec& spec = Spec(unit.type);
    std::string line = std::string(spec.name) + " " + TileName(map, unit.tile) + ": " +
                       std::to_string(DisplayHp(unit.hp)) + " hp, fuel " +
                       std::to_string(unit.fuel) + "/" + std::to_string(spec.fuel);
    if (spec.ammo > 0) {
        line += ", ammo " + std::to_string(unit.ammo) + "/" + std::to_string(spec.ammo);
    }
    if (unit.dived) {
        line += ", dived";
    }
    for (int c = 0; c < unit.cargo_count; c++) {
        line += c == 0 ? ", carries " : " and ";
        line += std::string(Spec(unit.cargo[c].type).name);
    }
    return line;
}

void DrawReport(State* state) {
    const Layout& l = state->layout;
    const Map& map = state->game.map;
    const MoveReport& r = state->report;
    float size = Ui(*state, 15);
    float small = Ui(*state, 13);
    FillRect(state->renderer, Rect{l.report.x, l.report.y, l.report.w, 1}, kPanelLine);
    float y = l.report.y + Ui(*state, 8);
    if (!r.valid) {
        std::string text = state->game.moves.empty() ? "Start position" : "Start of the game";
        DrawText(&state->font, l.report.x, y, size, kTextDim, text);
        DrawFocusLine(state, y + LineHeight(size) + LineHeight(small));
        return;
    }
    const Move& m = r.move;
    std::string head = std::to_string(state->ply) + ".  ";
    std::string line;
    std::string detail;
    switch (m.kind) {
    case MoveKind::EndTurn:
        line = "ends the turn";
        break;
    case MoveKind::Build:
        line = "builds " + UnitName(r.mover) + " at " + TileName(map, m.to);
        detail = "costs " + std::to_string(Spec(r.mover).cost);
        break;
    case MoveKind::Join:
        line = UnitName(r.mover) + " " + TileName(map, m.from) + " joins the one on " +
               TileName(map, m.to);
        break;
    case MoveKind::Load:
        line = UnitName(r.mover) + " " + TileName(map, m.from) + " boards the transport on " +
               TileName(map, m.to);
        break;
    case MoveKind::Unload:
        line = UnitName(r.mover) + " " + TileName(map, m.from) + " to " + TileName(map, m.to) +
               ", drops on " + TileName(map, m.target);
        break;
    case MoveKind::Dive:
    case MoveKind::Surface:
        line = UnitName(r.mover) + " " + TileName(map, m.from);
        if (m.to != m.from) {
            line += " to " + TileName(map, m.to);
        }
        line += m.kind == MoveKind::Dive ? ", dives" : ", surfaces";
        break;
    case MoveKind::Wait:
    case MoveKind::Attack:
    case MoveKind::Capture:
        line = UnitName(r.mover) + " " + TileName(map, m.from);
        if (m.to != m.from) {
            line += " to " + TileName(map, m.to);
        } else if (m.kind == MoveKind::Wait) {
            line += " holds";
        }
        break;
    }
    if (r.attack) {
        detail = "fires at " + UnitName(r.target) + " " + TileName(map, m.target) + ": " +
                 std::to_string(r.target_hp_before) + " > " + std::to_string(r.target_hp_after);
        if (r.target_hp_after == 0) {
            detail += ", destroyed";
        } else if (r.mover_hp_after == 0) {
            detail += ", lost to the counter";
        } else if (r.mover_hp_after < r.mover_hp_before) {
            detail += ", takes " + std::to_string(r.mover_hp_before - r.mover_hp_after) + " back";
        }
    }
    if (r.capture) {
        detail = r.capture_left == 0
                     ? "captures " + std::string(Spec(map.tiles[m.to]).name)
                     : "capturing, " + std::to_string(r.capture_left) + " points left";
    }
    float x = l.report.x;
    x += DrawText(&state->font, x, y, size, kTextFaint, head);
    x += DrawText(&state->font, x, y, size, kPlayer[r.side], kPlayerName[r.side]);
    DrawText(&state->font, x + Ui(*state, 6), y, size, kText, line);
    if (!detail.empty()) {
        Color color = r.attack ? Color{0xff, 0x9a, 0x9f} : kTextDim;
        DrawText(&state->font, l.report.x, y + LineHeight(size), small, color, detail);
    }
    DrawFocusLine(state, y + LineHeight(size) + LineHeight(small));
}

void DrawFooter(State* state) {
    const Layout& l = state->layout;
    float size = Ui(*state, 12);
    FillRect(state->renderer, Rect{l.footer.x, l.footer.y, l.footer.w, 1}, kPanelLine);
    float y = l.footer.y + Ui(*state, 5);
    DrawText(&state->font, l.footer.x, y, size, kTextFaint,
             "left right  move     up down  turn     home end");
    std::string second = state->mode == Mode::Play
                             ? "P  replay   E  end turn   O  engine opponent   S  save line   R  reload   A  analyse"
                             : "P  play from here     A  analyse     B  best turn";
    DrawText(&state->font, l.footer.x, y + LineHeight(size), size, kTextFaint, second);
}

} // namespace

std::string FormatEval(int eval) {
    if (eval > kWinThreshold) {
        return "+won";
    }
    if (eval < -kWinThreshold) {
        return "-won";
    }
    std::string sign = eval > 0 ? "+" : eval < 0 ? "-" : "";
    return sign + Thousands(std::abs(eval));
}

std::string FormatFunds(int funds) {
    std::string digits = std::to_string(funds < 0 ? -funds : funds);
    std::string out;
    int count = 0;
    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; i--) {
        out.insert(out.begin(), digits[static_cast<std::size_t>(i)]);
        if (++count % 3 == 0 && i > 0) {
            out.insert(out.begin(), ',');
        }
    }
    return (funds < 0 ? "-" : "") + out;
}

void DrawEvalBar(State* state) {
    const Rect& bar = state->layout.eval_bar;
    if (bar.w <= 0) {
        return;
    }
    float radius = Ui(*state, 4);
    int eval = 0;
    if (!ShownEvalForPlayer0(*state, state->ply, &eval)) {
        FillRoundRect(state->renderer, bar, radius, kPanelRaised);
        FillRect(state->renderer, Rect{bar.x, bar.y + bar.h * 0.5f - 1, bar.w, 2}, kPanelLine);
        return;
    }
    float share = 0.5f + 0.5f * std::tanh(static_cast<float>(eval) / kEvalBarScale);
    if (eval > kWinThreshold) {
        share = 1;
    } else if (eval < -kWinThreshold) {
        share = 0;
    }
    // player 1 from the top, player 0 from the bottom, like the board's two halves
    FillRoundRect(state->renderer, bar, radius, kPlayerDeep[1]);
    float split = std::round(bar.y + bar.h * (1 - share));
    Rect bottom{bar.x, split, bar.w, bar.y + bar.h - split};
    if (bottom.h > 0) {
        FillRoundRect(state->renderer, bottom, bottom.h < radius * 2 ? 0 : radius, kPlayer[0]);
        FillRect(state->renderer, Rect{bar.x, split, bar.w, std::fmin(radius, bottom.h)},
                 kPlayer[0]);
    }
    FillRect(state->renderer, Rect{bar.x, bar.y + bar.h * 0.5f, bar.w, 1}, WithAlpha(kInk, 140));
    FillRect(state->renderer, Rect{bar.x, split - 1, bar.w, 2}, kText);
}

void DrawSidePanel(State* state) {
    const Layout& l = state->layout;
    FillRect(state->renderer, l.panel, kPanel);
    FillRect(state->renderer, Rect{l.panel.x, 0, 1, l.height}, kPanelLine);
    DrawHeader(state);
    DrawReport(state);
    DrawEnginePanel(state);
    DrawEvalGraph(state);
    DrawMoveList(state);
    DrawFooter(state);
}

} // namespace shoal
