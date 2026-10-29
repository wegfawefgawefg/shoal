#include "search/search.hpp"

#include "search/eval.hpp"
#include "search/order.hpp"

#include <cstdio>
#include <cstring>

namespace shoal {

namespace {

constexpr int kSolveDepth = 30000;
constexpr int kInfinity = kWinScore + 1;
constexpr int kMaxQuiescePlies = 12;

// win scores count plies from the node, so a table hit means the same thing at any ply
int ToTable(int score, int ply) {
    if (score > kWinThreshold) {
        return score + ply;
    }
    if (score < -kWinThreshold) {
        return score - ply;
    }
    return score;
}

int FromTable(int score, int ply) {
    if (score > kWinThreshold) {
        return score - ply;
    }
    if (score < -kWinThreshold) {
        return score + ply;
    }
    return score;
}

bool OutOfBudget(Searcher* s) {
    if (s->stopped) {
        return true;
    }
    if ((s->nodes & 4095) != 0) {
        return false;
    }
    if (s->node_limit != 0 && s->nodes >= s->node_limit) {
        s->stopped = true;
    }
    if (s->timed && std::chrono::steady_clock::now() >= s->deadline) {
        s->stopped = true;
    }
    return s->stopped;
}

int Terminal(const Searcher& s, const Position& pos, int ply) {
    int score = ResultScore(pos.result, pos.side);
    if (s.solve) {
        return score > 0 ? 1 : (score < 0 ? -1 : 0);
    }
    if (score > 0) {
        return score - ply;
    }
    if (score < 0) {
        return score + ply;
    }
    return 0;
}

PlyScratch& ScratchAt(Searcher* s, int ply) {
    while (static_cast<int>(s->plies.size()) <= ply) {
        s->plies.emplace_back();
    }
    return s->plies[static_cast<std::size_t>(ply)];
}

// at the horizon the side to move may still finish its attacks and captures; ending the
// turn there is the stand-pat score
int Quiesce(Searcher* s, const Position& pos, int alpha, int beta, int ply, int qply) {
    s->nodes++;
    if (pos.result != Result::Ongoing) {
        return Terminal(*s, pos, ply);
    }
    int stand = Evaluate(*s->map, pos, pos.side);
    if (stand >= beta || qply >= kMaxQuiescePlies || OutOfBudget(s)) {
        return stand;
    }
    if (stand > alpha) {
        alpha = stand;
    }
    PlyScratch& scratch = ScratchAt(s, ply);
    MoveList& list = scratch.list;
    int* scores = scratch.scores;
    GenerateMoves(*s->map, pos, &list);
    ScoreMoves(*s->map, pos, list, nullptr, s->history.data(), scores);
    int best = stand;
    for (int i = 0; i < list.count; i++) {
        PickNext(&list, scores, i);
        const Move& move = list.moves[i];
        if (move.kind != MoveKind::Attack && move.kind != MoveKind::Capture) {
            break; // tactical moves sort first
        }
        Position child = pos;
        ApplyMove(*s->map, &child, move);
        int score;
        if (child.result != Result::Ongoing) {
            score = Terminal(*s, child, ply + 1);
        } else {
            score = Quiesce(s, child, alpha, beta, ply + 1, qply + 1);
        }
        if (s->stopped) {
            return 0;
        }
        if (score > best) {
            best = score;
        }
        if (score > alpha) {
            alpha = score;
        }
        if (alpha >= beta) {
            break;
        }
    }
    return best;
}

int Negamax(Searcher* s, const Position& pos, int depth, int alpha, int beta, int ply) {
    s->nodes++;
    if (pos.result != Result::Ongoing) {
        return Terminal(*s, pos, ply);
    }
    if (!s->solve && depth <= 0) {
        return Quiesce(s, pos, alpha, beta, ply, 0);
    }
    if (OutOfBudget(s)) {
        return 0;
    }

    int alpha_start = alpha;
    Move table_move;
    bool have_table_move = false;
    const TableEntry* entry = TableProbe(s->table, pos.hash);
    if (entry != nullptr) {
        table_move = entry->best;
        have_table_move = true;
        if (entry->depth >= depth) {
            int score = FromTable(entry->score, ply);
            if (entry->bound == Bound::Exact) {
                return score;
            }
            if (entry->bound == Bound::Lower && score >= beta) {
                return score;
            }
            if (entry->bound == Bound::Upper && score <= alpha) {
                return score;
            }
        }
    }

    PlyScratch& scratch = ScratchAt(s, ply);
    MoveList& list = scratch.list;
    int* scores = scratch.scores;
    GenerateMoves(*s->map, pos, &list);
    ScoreMoves(*s->map, pos, list, have_table_move ? &table_move : nullptr, s->history.data(),
               scores);

    int best_score = -kInfinity;
    Move best_move = list.moves[list.count - 1];
    for (int i = 0; i < list.count; i++) {
        PickNext(&list, scores, i);
        const Move& move = list.moves[i];
        if (move.kind == MoveKind::Wait && move.to == move.from) {
            // waiting in place is never better than leaving the unit free to act later
            continue;
        }
        Position child = pos;
        ApplyMove(*s->map, &child, move);
        int score;
        if (child.side == pos.side && child.result == Result::Ongoing) {
            score = Negamax(s, child, depth - 1, alpha, beta, ply + 1);
        } else if (child.result != Result::Ongoing) {
            score = Terminal(*s, child, ply + 1);
            if (child.side != pos.side) {
                score = -score;
            }
        } else {
            score = -Negamax(s, child, depth - 1, -beta, -alpha, ply + 1);
        }
        if (s->stopped) {
            return 0;
        }
        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
        if (score > alpha) {
            alpha = score;
        }
        if (alpha >= beta) {
            if (move.kind == MoveKind::Wait) {
                HistoryReward(s->history.data(), move, depth);
            }
            break;
        }
    }

    Bound bound = Bound::Exact;
    if (best_score <= alpha_start) {
        bound = Bound::Upper;
    } else if (best_score >= beta) {
        bound = Bound::Lower;
    }
    TableStore(&s->table, pos.hash, ToTable(best_score, ply), s->solve ? kSolveDepth : depth,
               bound, best_move);
    return best_score;
}

std::vector<Move> PrincipalLine(Searcher* s, const Position& root, int max_length) {
    std::vector<Move> line;
    Position pos = root;
    for (int i = 0; i < max_length && pos.result == Result::Ongoing; i++) {
        const TableEntry* entry = TableProbe(s->table, pos.hash);
        if (entry == nullptr || !IsLegal(*s->map, pos, entry->best)) {
            break;
        }
        line.push_back(entry->best);
        ApplyMove(*s->map, &pos, entry->best);
    }
    return line;
}

} // namespace

void SearcherInit(Searcher* searcher, const Map& map, int table_log2) {
    searcher->map = &map;
    TableResize(&searcher->table, table_log2);
    searcher->history.assign(static_cast<std::size_t>(kMaxTiles) * kMaxTiles, 0);
}

SearchResult Search(Searcher* s, const Position& root, const SearchLimits& limits) {
    SearchResult result;
    auto start = std::chrono::steady_clock::now();
    s->nodes = 0;
    s->stopped = false;
    s->solve = limits.solve;
    s->node_limit = limits.nodes;
    s->timed = limits.seconds > 0;
    s->deadline = start + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                              std::chrono::duration<double>(limits.seconds));
    s->table.age++;

    int first_depth = limits.solve ? kSolveDepth : 1;
    int last_depth = limits.solve ? kSolveDepth : limits.max_depth;
    for (int depth = first_depth; depth <= last_depth; depth++) {
        int window = limits.solve ? 1 : kInfinity;
        int score = Negamax(s, root, depth, -window, window, 0);
        double seconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        if (s->stopped) {
            break;
        }
        result.score = score;
        result.depth = depth;
        result.complete = true;
        const TableEntry* entry = TableProbe(s->table, root.hash);
        if (entry != nullptr) {
            result.best = entry->best;
        }
        result.pv = PrincipalLine(s, root, limits.solve ? 4000 : 64);
        if (!limits.quiet) {
            std::printf("depth %d score %d nodes %llu time %.2fs\n",
                        limits.solve ? -1 : depth, score,
                        static_cast<unsigned long long>(s->nodes), seconds);
            std::fflush(stdout);
        }
        if (score > kWinThreshold || score < -kWinThreshold) {
            break;
        }
    }
    result.nodes = s->nodes;
    result.seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    return result;
}

} // namespace shoal
