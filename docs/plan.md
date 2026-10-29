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

- Rules engine: all AW2 ground, air and sea units (no Dual Strike units), movement, combat
  with counters and ammo, fuel, capture, repair at cities/airports/ports, income, builds at
  bases/airports/ports, transports (APC, T-Copter, Lander, Cruiser), join, sub dive,
  rout/hq/day-limit endings. Fuzzed for board invariants, water maps included.
- Ply search + exact solver (`shoal search --solve`), bucketed table. Tiny results in `docs/findings.md`.
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

1. Viewer: keep hand-played lines as a variation next to the game (saving works: `S`).
2. Keep tuning on stevejobs; validate each candidate by match before adopting.
3. Why so few artillery in self-play, and the large first-player edge (findings).
4. Tactics inside a turn: small exact search over attack orders (who hits first) for the
   units that can reach the same targets.
5. Air and sea: tune the new unit values (`value_fighter` ...) and `fuel_risk` by SPSA on
   `--water` maps; check landers and battleships get built and used.
6. Opening books for fixed maps (stevejobs, long searches per turn start).
7. Learned eval: train a small net on planner/self-play positions (stevejobs).
8. Fog.
