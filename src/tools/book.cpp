#include "tools/book.hpp"

#include "io/map_file.hpp"
#include "search/planner.hpp"
#include "tools/commands.hpp"

#include <cstdio>
#include <cstdlib>
#include <gsexp/sexp.hpp>
#include <memory>
#include <sstream>

namespace shoal {

namespace {

std::string HashText(std::uint64_t hash) {
    char text[24];
    std::snprintf(text, sizeof(text), "%016llx", static_cast<unsigned long long>(hash));
    return text;
}

// "b1 b2, c3 build inf" -> moves
bool ParseTurn(const Map& map, std::string_view text, std::vector<Move>* moves) {
    moves->clear();
    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t comma = text.find(',', start);
        if (comma == std::string_view::npos) {
            comma = text.size();
        }
        std::string_view part = text.substr(start, comma - start);
        while (!part.empty() && part.front() == ' ') {
            part.remove_prefix(1);
        }
        if (!part.empty()) {
            Move move;
            if (!ParseMoveText(map, part, &move)) {
                return false;
            }
            moves->push_back(move);
        }
        start = comma + 1;
    }
    return true;
}

// one past the paren that closes the form opening at `start`; strings may hold parens
std::size_t FormEnd(const std::string& text, std::size_t start) {
    int depth = 0;
    bool in_string = false;
    for (std::size_t i = start; i < text.size(); i++) {
        char c = text[i];
        if (in_string) {
            if (c == '\\') {
                i++;
            } else if (c == '"') {
                in_string = false;
            }
            continue;
        }
        if (c == '"') {
            in_string = true;
        } else if (c == '(') {
            depth++;
        } else if (c == ')' && --depth == 0) {
            return i + 1;
        }
    }
    return std::string::npos;
}

struct BookNode {
    Position pos;
    int depth = 0;
};

} // namespace

bool LoadBook(const std::string& path, Book* book, std::string* error) {
    std::string text;
    if (!ReadWholeFile(path, &text)) {
        *error = "cannot read " + path;
        return false;
    }
    std::size_t map_start = text.find("(map");
    std::size_t map_end =
        map_start == std::string::npos ? std::string::npos : FormEnd(text, map_start);
    if (map_end == std::string::npos) {
        *error = "book has no map";
        return false;
    }
    if (!LoadMapText(text.substr(map_start, map_end - map_start), &book->map, error)) {
        return false;
    }
    gsexp::ParseResult parsed = gsexp::parse(text);
    if (!parsed.ok || parsed.root_count() == 0) {
        *error = "book does not parse";
        return false;
    }
    for (gsexp::Node form : parsed.root(0).children()) {
        if (!form.is_list() || !form.head().is_atom("entry") || form.child_count() < 4) {
            continue;
        }
        std::string hash_text(form.child_at(1).text());
        std::uint64_t hash = std::strtoull(hash_text.c_str(), nullptr, 16);
        std::vector<Move> moves;
        if (!ParseTurn(book->map, form.child_at(3).text(), &moves)) {
            *error = "bad turn in book entry " + hash_text;
            return false;
        }
        book->turns[hash] = moves;
        book->scores[hash] = std::atoi(std::string(form.child_at(2).text()).c_str());
    }
    return true;
}

std::string WriteBookText(const Book& book) {
    std::ostringstream out;
    out << "(book\n" << WriteMapText(book.map);
    for (const auto& [hash, moves] : book.turns) {
        auto score = book.scores.find(hash);
        out << "  (entry \"" << HashText(hash) << "\" "
            << (score == book.scores.end() ? 0 : score->second) << " "
            << gsexp::quote_string(TurnMovesText(book.map, moves)) << ")\n";
    }
    out << ")\n";
    return out.str();
}

bool BookTurn(const Book& book, const Map& map, const Position& pos, std::vector<Move>* moves) {
    auto found = book.turns.find(pos.hash);
    if (found == book.turns.end()) {
        return false;
    }
    Position check = pos;
    for (const Move& move : found->second) {
        if (!IsLegal(map, check, move)) {
            return false;
        }
        ApplyMove(map, &check, move);
    }
    *moves = found->second;
    return true;
}

// shoal book <map> --turns N --time S --threads T --branch K --out book.sexp: from the start,
// every turn start to N turns deep gets a long search; the tree follows the best turn and the
// next K-1 candidate turns of the beam
int RunBook(int argc, char** argv) {
    if (argc < 1) {
        std::fprintf(stderr, "usage: shoal book <map> [--turns N] [--time S] [--threads T] "
                             "[--branch K] --out book.sexp\n");
        return 1;
    }
    int max_turns = 4;
    double seconds = 5.0;
    int threads = 8;
    int branch = 2;
    std::string out_path;
    for (int i = 1; i + 1 < argc; i += 2) {
        std::string arg = argv[i];
        if (arg == "--turns") {
            max_turns = std::atoi(argv[i + 1]);
        } else if (arg == "--time") {
            seconds = std::atof(argv[i + 1]);
        } else if (arg == "--threads") {
            threads = std::atoi(argv[i + 1]);
        } else if (arg == "--branch") {
            branch = std::atoi(argv[i + 1]);
        } else if (arg == "--out") {
            out_path = argv[i + 1];
        }
    }
    auto book = std::make_unique<Book>();
    std::string error;
    if (!LoadMapFile(argv[0], &book->map, &error)) {
        std::fprintf(stderr, "%s: %s\n", argv[0], error.c_str());
        return 1;
    }
    const Map& map = book->map;
    auto planner = std::make_unique<Planner>();
    PlannerInit(planner.get(), map);
    std::vector<BookNode> queue(1);
    SetupStart(map, &queue[0].pos);
    for (std::size_t q = 0; q < queue.size(); q++) {
        BookNode node = queue[q];
        if (book->turns.count(node.pos.hash) != 0 || node.pos.result != Result::Ongoing) {
            continue;
        }
        PlannerLimits limits;
        limits.seconds = seconds;
        limits.threads = threads;
        limits.quiet = true;
        PlannerResult result = PlanTurn(planner.get(), node.pos, limits);
        book->turns[node.pos.hash] = result.turn.moves;
        book->scores[node.pos.hash] = result.score;
        std::printf("turn %d day %d p%d score %d turns %d: %s\n", node.depth, node.pos.day,
                    node.pos.side, result.score, result.turns,
                    TurnMovesText(map, result.turn.moves).c_str());
        std::fflush(stdout);
        if (node.depth + 1 >= max_turns) {
            continue;
        }
        // the best turn, then the beam's next candidates as the other lines worth knowing
        std::vector<std::vector<Move>> lines = {result.turn.moves};
        std::vector<TurnPlan> candidates = BeamTurns(planner.get(), node.pos, branch + 1);
        for (const TurnPlan& plan : candidates) {
            if (static_cast<int>(lines.size()) >= branch) {
                break;
            }
            if (plan.moves != result.turn.moves) {
                lines.push_back(plan.moves);
            }
        }
        for (const std::vector<Move>& line : lines) {
            BookNode child;
            child.pos = node.pos;
            for (const Move& move : line) {
                ApplyMove(map, &child.pos, move);
            }
            if (child.pos.result == Result::Ongoing) {
                ApplyMove(map, &child.pos, Move{});
            }
            child.depth = node.depth + 1;
            queue.push_back(child);
        }
    }
    if (out_path.empty() || !WriteWholeFile(out_path, WriteBookText(*book))) {
        std::fprintf(stderr, "cannot write the book\n");
        return 1;
    }
    std::printf("%zu positions -> %s\n", book->turns.size(), out_path.c_str());
    return 0;
}

} // namespace shoal
