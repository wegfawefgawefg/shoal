#include "unit_parts.hpp"

namespace shoal {

namespace {

// rotor on a mast, seen edge on
void AddRotor(Art* art, float x0, float x1, float y, float mast_x) {
    AddRect(art, Tone::Metal, mast_x - 0.02f, y, 0.04f, 0.1f);
    AddCapsule(art, Tone::Dark, {x0, y}, {x1, y}, 0.035f);
    AddCircle(art, Tone::Metal, mast_x, y, 0.03f, true);
}

// a ship's paint line where the hull meets the water
void AddWaterline(Art* art, float x0, float x1, float y) {
    AddRect(art, Tone::Deep, x0, y, x1 - x0, 0.035f, true);
}

} // namespace

void BuildFighter(Art* art) {
    AddPoly(art, Tone::Body, {{0.1f, 0.47f}, {0.14f, 0.22f}, {0.25f, 0.22f}, {0.34f, 0.45f}});
    AddRect(art, Tone::Metal, 0.03f, 0.46f, 0.07f, 0.08f);
    AddPoly(art, Tone::Body,
            {{0.08f, 0.45f},
             {0.6f, 0.41f},
             {0.82f, 0.44f},
             {0.97f, 0.5f},
             {0.82f, 0.55f},
             {0.08f, 0.56f}});
    AddPoly(art, Tone::Glass, {{0.56f, 0.43f}, {0.64f, 0.34f}, {0.76f, 0.35f}, {0.84f, 0.45f}});
    // swept wing toward the viewer, a missile under it
    AddPoly(art, Tone::Deep, {{0.28f, 0.52f}, {0.6f, 0.52f}, {0.44f, 0.7f}, {0.3f, 0.7f}});
    AddCapsule(art, Tone::Light, {0.34f, 0.74f}, {0.54f, 0.74f}, 0.045f);
    AddRect(art, Tone::Light, 0.14f, 0.48f, 0.5f, 0.025f, true);
    AddRect(art, Tone::Deep, 0.16f, 0.27f, 0.08f, 0.025f, true);
}

void BuildBomber(Art* art) {
    AddPoly(art, Tone::Body, {{0.06f, 0.46f}, {0.1f, 0.18f}, {0.22f, 0.18f}, {0.32f, 0.42f}});
    AddPoly(art, Tone::Body,
            {{0.05f, 0.44f},
             {0.66f, 0.37f},
             {0.86f, 0.4f},
             {0.96f, 0.5f},
             {0.88f, 0.6f},
             {0.06f, 0.6f}});
    AddPoly(art, Tone::Glass, {{0.84f, 0.42f}, {0.95f, 0.5f}, {0.84f, 0.52f}}, true);
    // broad wing with an engine pod, a bomb in the belly
    AddPoly(art, Tone::Deep, {{0.32f, 0.54f}, {0.68f, 0.54f}, {0.58f, 0.74f}, {0.38f, 0.74f}});
    AddCapsule(art, Tone::Metal, {0.42f, 0.66f}, {0.62f, 0.66f}, 0.08f);
    AddCircle(art, Tone::Dark, 0.63f, 0.66f, 0.03f, true);
    AddCapsule(art, Tone::Dark, {0.18f, 0.66f}, {0.3f, 0.66f}, 0.07f);
    AddRect(art, Tone::Light, 0.1f, 0.47f, 0.7f, 0.025f, true);
    AddRect(art, Tone::Deep, 0.12f, 0.24f, 0.09f, 0.025f, true);
}

void BuildBCopter(Art* art) {
    AddPoly(art, Tone::Body, {{0.04f, 0.28f}, {0.12f, 0.28f}, {0.16f, 0.46f}, {0.07f, 0.46f}});
    AddCapsule(art, Tone::Body, {0.1f, 0.44f}, {0.4f, 0.47f}, 0.07f);
    AddCircle(art, Tone::Dark, 0.08f, 0.34f, 0.045f);
    AddCapsule(art, Tone::Dark, {0.36f, 0.72f}, {0.82f, 0.72f}, 0.03f);
    AddCapsule(art, Tone::Dark, {0.44f, 0.58f}, {0.44f, 0.72f}, 0.025f);
    AddCapsule(art, Tone::Dark, {0.72f, 0.58f}, {0.72f, 0.72f}, 0.025f);
    AddRotor(art, 0.12f, 0.88f, 0.24f, 0.52f);
    AddPoly(art, Tone::Body,
            {{0.3f, 0.48f},
             {0.38f, 0.36f},
             {0.66f, 0.34f},
             {0.88f, 0.47f},
             {0.82f, 0.6f},
             {0.38f, 0.61f}});
    AddPoly(art, Tone::Glass, {{0.64f, 0.37f}, {0.84f, 0.47f}, {0.66f, 0.48f}}, true);
    // stub wing and rocket pod
    AddCapsule(art, Tone::Deep, {0.44f, 0.56f}, {0.66f, 0.56f}, 0.07f);
    AddCircle(art, Tone::Dark, 0.67f, 0.56f, 0.025f, true);
}

void BuildTCopter(Art* art) {
    AddPoly(art, Tone::Body, {{0.03f, 0.27f}, {0.1f, 0.27f}, {0.14f, 0.44f}, {0.06f, 0.44f}});
    AddCapsule(art, Tone::Body, {0.08f, 0.42f}, {0.24f, 0.44f}, 0.08f);
    AddWheel(art, 0.3f, 0.7f, 0.05f);
    AddWheel(art, 0.76f, 0.7f, 0.05f);
    AddRotor(art, 0.1f, 0.9f, 0.22f, 0.5f);
    AddPoly(art, Tone::Body,
            {{0.16f, 0.36f}, {0.7f, 0.32f}, {0.9f, 0.44f}, {0.9f, 0.62f}, {0.16f, 0.64f}});
    AddPoly(art, Tone::Glass, {{0.72f, 0.36f}, {0.88f, 0.45f}, {0.72f, 0.47f}}, true);
    for (int i = 0; i < 3; i++) {
        AddRect(art, Tone::Glass, 0.26f + 0.13f * static_cast<float>(i), 0.42f, 0.08f, 0.06f, true);
    }
    AddRect(art, Tone::Deep, 0.18f, 0.56f, 0.7f, 0.03f, true);
}

void BuildBattleship(Art* art) {
    AddCapsule(art, Tone::Metal, {0.5f, 0.38f}, {0.5f, 0.16f}, 0.03f);
    AddCapsule(art, Tone::Dark, {0.42f, 0.22f}, {0.58f, 0.22f}, 0.03f);
    // twin barrels fore and aft
    AddCapsule(art, Tone::Deep, {0.72f, 0.5f}, {0.97f, 0.45f}, 0.035f);
    AddCapsule(art, Tone::Deep, {0.72f, 0.54f}, {0.97f, 0.5f}, 0.035f);
    AddCapsule(art, Tone::Deep, {0.28f, 0.5f}, {0.04f, 0.45f}, 0.035f);
    AddPoly(art, Tone::Body, {{0.03f, 0.58f}, {0.97f, 0.58f}, {0.86f, 0.78f}, {0.1f, 0.78f}});
    AddPoly(art, Tone::Body, {{0.38f, 0.58f}, {0.62f, 0.58f}, {0.58f, 0.38f}, {0.42f, 0.38f}});
    AddPoly(art, Tone::Body, {{0.64f, 0.58f}, {0.82f, 0.58f}, {0.79f, 0.47f}, {0.67f, 0.47f}});
    AddPoly(art, Tone::Body, {{0.18f, 0.58f}, {0.36f, 0.58f}, {0.33f, 0.47f}, {0.21f, 0.47f}});
    AddRect(art, Tone::Glass, 0.45f, 0.42f, 0.1f, 0.04f, true);
    AddRect(art, Tone::Light, 0.42f, 0.5f, 0.16f, 0.025f, true);
    AddWaterline(art, 0.08f, 0.88f, 0.7f);
}

void BuildCruiser(Art* art) {
    AddCapsule(art, Tone::Metal, {0.48f, 0.36f}, {0.48f, 0.18f}, 0.03f);
    AddCapsule(art, Tone::Dark, {0.4f, 0.2f}, {0.56f, 0.2f}, 0.035f);
    // twin anti-air guns up front, a missile box aft
    AddCapsule(art, Tone::Deep, {0.7f, 0.54f}, {0.82f, 0.38f}, 0.03f);
    AddCapsule(art, Tone::Deep, {0.75f, 0.55f}, {0.87f, 0.39f}, 0.03f);
    AddPoly(art, Tone::Body, {{0.05f, 0.6f}, {0.98f, 0.55f}, {0.84f, 0.78f}, {0.12f, 0.78f}});
    AddPoly(art, Tone::Body, {{0.34f, 0.6f}, {0.62f, 0.6f}, {0.58f, 0.44f}, {0.38f, 0.44f}});
    AddPoly(art, Tone::Body, {{0.41f, 0.45f}, {0.55f, 0.45f}, {0.53f, 0.35f}, {0.43f, 0.35f}});
    AddBox(art, Tone::Deep, 0.22f, 0.54f, 0.16f, 0.08f, 0.35f);
    AddCircle(art, Tone::Body, 0.74f, 0.56f, 0.05f);
    AddRect(art, Tone::Glass, 0.42f, 0.48f, 0.12f, 0.035f, true);
    AddWaterline(art, 0.1f, 0.88f, 0.7f);
}

void BuildLander(Art* art) {
    AddRect(art, Tone::Metal, 0.12f, 0.22f, 0.05f, 0.1f);
    AddPoly(art, Tone::Body,
            {{0.04f, 0.52f}, {0.84f, 0.52f}, {0.97f, 0.6f}, {0.9f, 0.78f}, {0.08f, 0.78f}});
    AddPoly(art, Tone::Body, {{0.06f, 0.53f}, {0.28f, 0.53f}, {0.26f, 0.32f}, {0.08f, 0.32f}});
    // the bow ramp, raised
    AddPoly(art, Tone::Deep, {{0.8f, 0.52f}, {0.86f, 0.42f}, {0.95f, 0.5f}, {0.9f, 0.6f}});
    AddRect(art, Tone::Deep, 0.32f, 0.55f, 0.46f, 0.06f, true);
    AddRect(art, Tone::Glass, 0.12f, 0.36f, 0.11f, 0.045f, true);
    AddWaterline(art, 0.08f, 0.9f, 0.7f);
}

void BuildSub(Art* art) {
    AddCircle(art, Tone::Metal, 0.08f, 0.64f, 0.045f);
    AddCapsule(art, Tone::Metal, {0.55f, 0.42f}, {0.55f, 0.24f}, 0.025f);
    AddCapsule(art, Tone::Metal, {0.55f, 0.25f}, {0.61f, 0.25f}, 0.025f);
    AddCapsule(art, Tone::Body, {0.16f, 0.64f}, {0.86f, 0.64f}, 0.2f);
    AddPoly(art, Tone::Body, {{0.4f, 0.58f}, {0.62f, 0.58f}, {0.6f, 0.4f}, {0.45f, 0.4f}});
    AddRect(art, Tone::Deep, 0.36f, 0.48f, 0.3f, 0.03f, true);
    AddRect(art, Tone::Light, 0.18f, 0.6f, 0.62f, 0.025f, true);
    AddWaterline(art, 0.1f, 0.92f, 0.68f);
}

} // namespace shoal
