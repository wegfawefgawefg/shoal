#include "rules/terrain.hpp"

namespace shoal {

bool RepairsGround(Terrain terrain) {
    return terrain == Terrain::City || terrain == Terrain::Base || terrain == Terrain::Hq;
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
