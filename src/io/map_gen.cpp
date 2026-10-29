#include "io/map_gen.hpp"

#include <string>

namespace shoal {

namespace {

struct GenRng {
    std::uint64_t state;
    std::uint64_t Next() {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return state;
    }
    int Below(int n) {
        return static_cast<int>(Next() % static_cast<std::uint64_t>(n));
    }
};

int Mirror(const Map& map, int tile) {
    return map.width * map.height - 1 - tile;
}

void SetPair(Map* map, int tile, Terrain terrain) {
    map->tiles[tile] = terrain;
    map->tiles[Mirror(*map, tile)] = terrain;
}

// treads reach every property from the first hq
bool Connected(const Map& map, int start) {
    int n = map.width * map.height;
    std::uint8_t seen[kMaxTiles] = {};
    int queue[kMaxTiles];
    int head = 0;
    int tail = 0;
    queue[tail++] = start;
    seen[start] = 1;
    while (head < tail) {
        int tile = queue[head++];
        int around[4];
        int count = Neighbours(map, tile, around);
        for (int i = 0; i < count; i++) {
            int to = around[i];
            if (seen[to] || MoveCost(map.tiles[to], MoveType::Treads) == 0) {
                continue;
            }
            seen[to] = 1;
            queue[tail++] = to;
        }
    }
    for (int tile = 0; tile < n; tile++) {
        if (Spec(map.tiles[tile]).property && !seen[tile]) {
            return false;
        }
    }
    return true;
}

bool FreeFor(const Map& map, int tile) {
    return map.tiles[tile] == Terrain::Plain && map.tiles[Mirror(map, tile)] == Terrain::Plain &&
           tile != Mirror(map, tile);
}

// a lake around the centre, long along the other diagonal so it sits between the armies;
// symmetric because the shape is. Land next to the water becomes shoal
void AddLake(GenRng* rng, Map* map) {
    int n = map->width * map->height;
    double cx = (map->width - 1) * 0.5;
    double cy = (map->height - 1) * 0.5;
    int side = map->width < map->height ? map->width : map->height;
    double length = side * (0.26 + rng->Below(10) * 0.012);
    double breadth = side * (0.11 + rng->Below(6) * 0.014);
    if (breadth < 1.0) {
        breadth = 1.0;
    }
    for (int tile = 0; tile < n; tile++) {
        double dx = TileX(*map, tile) - cx;
        double dy = TileY(*map, tile) - cy;
        double along = (dx - dy) * 0.7071;
        double across = (dx + dy) * 0.7071;
        double r = along * along / (length * length) + across * across / (breadth * breadth);
        if (r <= 1.0 && map->tiles[tile] == Terrain::Plain &&
            map->tiles[Mirror(*map, tile)] == Terrain::Plain) {
            SetPair(map, tile, Terrain::Sea);
        }
    }
    for (int tile = 0; tile < n; tile++) {
        if (map->tiles[tile] != Terrain::Plain) {
            continue;
        }
        int around[4];
        int count = Neighbours(*map, tile, around);
        for (int i = 0; i < count; i++) {
            if (map->tiles[around[i]] == Terrain::Sea) {
                SetPair(map, tile, Terrain::Shoal);
                break;
            }
        }
    }
    // a reef pair out in the water now and then
    for (int tries = 0; tries < 40; tries++) {
        int tile = rng->Below(n);
        if (map->tiles[tile] != Terrain::Sea || tile == Mirror(*map, tile)) {
            continue;
        }
        int around[4];
        int count = Neighbours(*map, tile, around);
        bool open = count == 4;
        for (int i = 0; i < count; i++) {
            open = open && map->tiles[around[i]] == Terrain::Sea;
        }
        if (open && rng->Below(2) == 0) {
            SetPair(map, tile, Terrain::Reef);
        }
        break;
    }
}

// the shore tile nearest the hq becomes its port
bool AddPort(Map* map, int hq) {
    int n = map->width * map->height;
    int best = -1;
    for (int tile = 0; tile < n; tile++) {
        if (map->tiles[tile] != Terrain::Shoal || tile == Mirror(*map, tile)) {
            continue;
        }
        if (Distance(*map, tile, hq) >= Distance(*map, tile, Mirror(*map, hq))) {
            continue; // the other side's shore
        }
        if (best < 0 || Distance(*map, tile, hq) < Distance(*map, best, hq)) {
            best = tile;
        }
    }
    if (best < 0) {
        return false;
    }
    SetPair(map, best, Terrain::Port);
    return true;
}

bool TryGenerate(const MapGenOptions& options, GenRng* rng, Map* map) {
    *map = Map{};
    map->width = options.width;
    map->height = options.height;
    map->day_limit = options.day_limit;
    map->production = true;
    for (UnitType type : {UnitType::Infantry, UnitType::Mech, UnitType::Recon, UnitType::Tank,
                          UnitType::MdTank, UnitType::Neotank, UnitType::Artillery,
                          UnitType::Rockets, UnitType::AntiAir, UnitType::Apc}) {
        map->buildable[static_cast<int>(type)] = true;
    }
    if (options.water) {
        for (UnitType type : {UnitType::Missiles, UnitType::Fighter, UnitType::Bomber,
                              UnitType::BCopter, UnitType::TCopter, UnitType::Battleship,
                              UnitType::Cruiser, UnitType::Lander, UnitType::Sub}) {
            map->buildable[static_cast<int>(type)] = true;
        }
    }
    int n = map->width * map->height;
    for (int tile = 0; tile < n; tile++) {
        map->tiles[tile] = Terrain::Plain;
    }

    // hq near the top left corner, bases next to it
    int hq = TileAt(*map, rng->Below(2), rng->Below(2));
    SetPair(map, hq, Terrain::Hq);
    int bases_placed = 0;
    for (int tries = 0; tries < 100 && bases_placed < options.bases_per_side; tries++) {
        int x = TileX(*map, hq) + rng->Below(4);
        int y = TileY(*map, hq) + rng->Below(4);
        if (x >= map->width || y >= map->height) {
            continue;
        }
        int tile = TileAt(*map, x, y);
        if (Distance(*map, tile, hq) < 1 || !FreeFor(*map, tile)) {
            continue;
        }
        SetPair(map, tile, Terrain::Base);
        bases_placed++;
    }
    if (options.water) {
        bool airport = false;
        for (int tries = 0; tries < 100 && !airport; tries++) {
            int x = TileX(*map, hq) + rng->Below(5);
            int y = TileY(*map, hq) + rng->Below(5);
            if (x >= map->width || y >= map->height) {
                continue;
            }
            int tile = TileAt(*map, x, y);
            if (Distance(*map, tile, hq) < 2 || !FreeFor(*map, tile)) {
                continue;
            }
            SetPair(map, tile, Terrain::Airport);
            airport = true;
        }
        IndexProperties(map); // neighbour lists for the shore
        AddLake(rng, map);
        if (!airport || !AddPort(map, hq)) {
            return false;
        }
    }

    // neutral cities over the whole board
    int cities = options.cities_per_side > 0 ? options.cities_per_side : n / 24 + 1;
    for (int tries = 0; tries < 1000 && cities > 0; tries++) {
        int tile = rng->Below(n);
        if (!FreeFor(*map, tile)) {
            continue;
        }
        SetPair(map, tile, Terrain::City);
        cities--;
    }

    // a road from the hq toward the middle, then terrain
    int x = TileX(*map, hq);
    int y = TileY(*map, hq);
    for (int steps = 0; steps < map->width + map->height; steps++) {
        if (rng->Below(2) == 0 && x < map->width / 2) {
            x++;
        } else if (y < map->height / 2) {
            y++;
        } else {
            x++;
        }
        if (x >= map->width || y >= map->height) {
            break;
        }
        int tile = TileAt(*map, x, y);
        if (FreeFor(*map, tile)) {
            SetPair(map, tile, Terrain::Road);
        }
    }
    for (int tile = 0; tile < n / 2; tile++) {
        if (!FreeFor(*map, tile)) {
            continue;
        }
        int roll = rng->Below(100);
        if (roll < 14) {
            SetPair(map, tile, Terrain::Wood);
        } else if (roll < 20) {
            SetPair(map, tile, Terrain::Mountain);
        }
    }

    IndexProperties(map);
    for (int prop = 0; prop < map->property_count; prop++) {
        map->start_owner[prop] = kNeutral;
        int tile = map->property_tiles[prop];
        Terrain terrain = map->tiles[tile];
        if (terrain != Terrain::Hq && !IsFactory(terrain)) {
            continue;
        }
        // the hq's half of the board is player 0's
        bool near_first = Distance(*map, tile, hq) < Distance(*map, tile, Mirror(*map, hq));
        map->start_owner[prop] = near_first ? 0 : 1;
    }
    map->name = "gen" + std::to_string(options.width) + "x" + std::to_string(options.height) + "-" +
                std::to_string(options.seed);
    return Connected(*map, hq);
}

} // namespace

void GenerateMap(const MapGenOptions& options, Map* map) {
    GenRng rng{options.seed * 0x9e3779b97f4a7c15ull + 0x1234567ull};
    for (int i = 0; i < 8; i++) {
        rng.Next();
    }
    for (int attempt = 0; attempt < 100; attempt++) {
        if (TryGenerate(options, &rng, map)) {
            return;
        }
    }
}

} // namespace shoal
