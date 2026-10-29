#include "rules/terrain.hpp"

namespace shoal {

bool RepairsUnit(Terrain terrain, UnitType type) {
    switch (DomainOf(type)) {
    case Domain::Ground:
        return terrain == Terrain::City || terrain == Terrain::Base || terrain == Terrain::Hq;
    case Domain::Air:
        return terrain == Terrain::Airport;
    case Domain::Sea:
        return terrain == Terrain::Port;
    }
    return false;
}

bool BuildsUnit(Terrain terrain, UnitType type) {
    switch (DomainOf(type)) {
    case Domain::Ground:
        return terrain == Terrain::Base;
    case Domain::Air:
        return terrain == Terrain::Airport;
    case Domain::Sea:
        return terrain == Terrain::Port;
    }
    return false;
}

bool IsFactory(Terrain terrain) {
    return terrain == Terrain::Base || terrain == Terrain::Airport || terrain == Terrain::Port;
}

bool TerrainFromGlyph(char glyph, Terrain* out) {
    for (int i = 0; i < kTerrainCount; i++) {
        if (kTerrainSpecs[i].glyph == glyph) {
            *out = static_cast<Terrain>(i);
            return true;
        }
    }
    return false;
}

} // namespace shoal
