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
  rout/hq/day-limit endings. Fuzzed for board invariants, water maps included: `shoal fuzz` (1.2M random games
  on random maps with every unit, 320M moves, file round trips, 0 failures, 2026-10-29).
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

## Research (owner, 2026-10-29)

Each answers a question in `docs/guide/index.html` section 10; results go into the guide.

1. Fog: tiny fog games solved with hidden-information methods; does fog end the standoff?
2. Artillery and range: exact solves of inf+art vs inf+inf, rockets vs infantry, a city
   between two defended lines.
3. No day limit: HQ and rout as the only endings; retune and compare unit values and results.
4. Value table: cities, the first move, unit types, terrain, each as "funds that make it even".
5. Rule variants for a fair, combat-driven 1v1: fixed armies without production, combat win
   conditions, second-player compensation, simultaneous turns. Measured by self-play: first
   player win rate, draw rate, game length, comebacks.

## Queue

1. Viewer: keep hand-played lines as a variation next to the game (saving works: `S`).
2. Keep tuning on stevejobs; validate each candidate by match before adopting.
3. Why so few artillery in self-play, and the large first-player edge (findings).
4. Tactics inside a turn: small exact search over attack orders (who hits first) for the
   units that can reach the same targets.
5. Air and sea: tune the new unit values (`value_fighter` ...) and `fuel_risk` by SPSA on
   `--water` maps; check landers and battleships get built and used.
6. Solver: distance-to-win lines (fastest win, longest defence) for the solved games.
7. Opening books: `shoal book` exists; build books for the maps the owner plays on stevejobs.
8. Learned eval: train a small net on planner/self-play positions (stevejobs).
9. Fog.
