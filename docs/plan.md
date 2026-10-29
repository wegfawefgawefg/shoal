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
  income, builds, join, APC (load, drop, supply), rout/hq/day-limit endings. Up to 32x32
  and 128 units. Fuzzed for board invariants.
- Air and sea units: in progress on branch `airsea`.
- Ply search + exact solver (`shoal search --solve`). Tiny results in `docs/findings.md`.
- Turn planner: beats the ply search and greedy; deeper is better (findings). ~30k candidate
  turns/s on a 16x16 mid-game (5-7 turns deep in 1 s), threaded root search for analysis.
- Map generator, match harness, SPSA tuner (run 2 adopted; see findings), `shoal stats`.
- Viewer: replay with eval bar, last-move marks, move list; minimal play mode (`P`).
- Viewer play mode has an engine opponent (`O`).
- Learning tools: `shoal stats`, `shoal dataset` + `scripts/fit_values.py` (logistic fit of
  unit worth from game results).
- Analysis: `shoal analyse` scores every turn start and marks inaccuracies, mistakes and
  blunders into the game file; the viewer draws the eval graph, the marks, the engine's best
  turn as arrows, and runs the planner live on any position (`A`).

## Queue

1. Viewer: keep hand-played lines as a variation next to the game, save them.
2. Keep tuning (SPSA run 2 on stevejobs), validate each candidate by match before adopting.
3. Merge air/sea; retune with them; water maps in the generator.
4. Tactics inside a turn: small exact search over attack orders (who hits first) for the
   units that can reach the same targets.
5. Solver speed: tried partial-order reduction keyed into the table: worse (it splits
   transpositions the table already merges). Next idea: a bigger table with buckets.
6. Opening books for fixed maps (stevejobs, long searches per turn start).
7. Learned eval: train a small net on planner/self-play positions (stevejobs).
8. Fog.
