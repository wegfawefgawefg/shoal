#include "layout.hpp"

#include "replay.hpp"

#include <algorithm>
#include <cmath>

namespace shoal {

namespace {

constexpr float kLabelSpace = 18;
constexpr float kEvalBarWidth = 16;
constexpr float kEvalBarGap = 8;
constexpr float kMaxTile = 128;

} // namespace

float Ui(const State& state, float px) {
    return std::round(px * state.layout.ui);
}

void ComputeLayout(State* state, float width, float height) {
    Layout& l = state->layout;
    const Map& map = state->game.map;
    l.width = width;
    l.height = height;
    l.ui = std::clamp(std::min(height / 800.0f, width / 1200.0f), 0.8f, 2.4f);

    // side panel on the right
    float panel_w = std::clamp(width * 0.3f, Ui(*state, 300), Ui(*state, 420));
    if (panel_w > width * 0.5f) {
        panel_w = std::floor(width * 0.5f);
    }
    l.panel = Rect{width - panel_w, 0, panel_w, height};
    float pad = Ui(*state, 16);
    float header_h = Ui(*state, 168);
    float report_h = Ui(*state, 64);
    float footer_h = Ui(*state, 44);
    l.header = Rect{l.panel.x + pad, pad, panel_w - 2 * pad, header_h};
    l.report = Rect{l.panel.x + pad, l.header.y + header_h, panel_w - 2 * pad, report_h};
    l.footer = Rect{l.panel.x + pad, height - footer_h, panel_w - 2 * pad, footer_h - pad * 0.5f};
    float list_y = l.report.y + report_h + Ui(*state, 8);
    l.move_list = Rect{l.panel.x + pad, list_y, panel_w - 2 * pad,
                       std::max(0.0f, l.footer.y - list_y - Ui(*state, 8))};

    // board: as big as fits, whole pixels per tile, centred in what is left
    float bar = HasEvals(*state) ? Ui(*state, kEvalBarWidth) + Ui(*state, kEvalBarGap) : 0;
    float label = Ui(*state, kLabelSpace);
    float left = pad + bar + label;
    float top = pad + label;
    float avail_w = l.panel.x - pad - left;
    float avail_h = height - pad - top;
    float tile = 8;
    if (map.width > 0 && map.height > 0) {
        tile = std::floor(std::min(avail_w / static_cast<float>(map.width),
                                   avail_h / static_cast<float>(map.height)));
    }
    tile = std::clamp(tile, 8.0f, Ui(*state, kMaxTile));
    l.tile = tile;
    float board_w = tile * static_cast<float>(map.width);
    float board_h = tile * static_cast<float>(map.height);
    l.board = Rect{std::floor(left + (avail_w - board_w) * 0.5f),
                   std::floor(top + (avail_h - board_h) * 0.5f), board_w, board_h};
    l.eval_bar = Rect{};
    if (bar > 0) {
        l.eval_bar = Rect{l.board.x - label - bar, l.board.y, Ui(*state, kEvalBarWidth), board_h};
    }
}

int TileAtPoint(const State& state, float x, float y) {
    const Layout& l = state.layout;
    if (!Contains(l.board, x, y) || l.tile <= 0) {
        return -1;
    }
    int tx = static_cast<int>((x - l.board.x) / l.tile);
    int ty = static_cast<int>((y - l.board.y) / l.tile);
    if (tx < 0 || ty < 0 || tx >= state.game.map.width || ty >= state.game.map.height) {
        return -1;
    }
    return TileAt(state.game.map, tx, ty);
}

Rect TileRect(const State& state, int tile) {
    const Layout& l = state.layout;
    float x = l.board.x + static_cast<float>(TileX(state.game.map, tile)) * l.tile;
    float y = l.board.y + static_cast<float>(TileY(state.game.map, tile)) * l.tile;
    return Rect{x, y, l.tile, l.tile};
}

} // namespace shoal
