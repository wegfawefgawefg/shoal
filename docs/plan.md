# Plan

## Goals (owner, 2026-10-28)

1. See what optimal play looks like: solve tiny games exactly.
2. Grow the map size the engine handles, toward ~16x16 maps people play; seconds per good
   answer is fine.
3. Add the other units (air, sea, transports).
4. Optimise throughout, then learn about AW from it: "the most high aw play ever".
5. A native viewer like a lichess analysis board: replay first; playing moves in it later,
   so the owner can check what the eval thinks of a move.

Scope: AW2 tier rules (AWBW), no luck, no COs, production yes, fog later.

## Status

- Rules engine: ground units, movement, combat with counters and ammo, capture, repair,
  income, builds, rout/hq/day-limit endings. Fuzzed for board invariants.
- Ply search + exact solver (`shoal search --solve`). Tiny results in `docs/findings.md`.
- Turn planner: beats the ply search and greedy; deeper is better (findings).
- Map generator, match harness, SPSA tuner.
- Viewer: in progress on branch `viewer`.

## Queue

1. Merge the viewer; replay engine games with evals.
2. Tune planner weights (SPSA run on stevejobs), then re-check against the defaults.
3. Planner on 16x16 with big armies: profile, cut costs (threat map updates, wider delta
   scoring), measure depth reached.
4. Tactics inside a turn: small exact search over attack orders (who hits first) for the
   units that can reach the same targets.
5. Solver speed: partial-order reduction of independent unit actions; bigger table.
6. Viewer play mode; engine analysis in the viewer (eval bar live, best line, blunders).
7. APC load/unload and supply, join, then air and sea.
8. Learned eval: train a small net on planner/self-play positions (stevejobs).
9. Fog.
