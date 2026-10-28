# Engine

Two searches share the rules code.

## Ply search (`src/search/search.cpp`)

Negamax alpha-beta where one ply is one unit action (move + wait/attack/capture, a build, or
`end`). The side to move keeps moving until `end`, so the sign flips only at `end`.

- Transposition table keyed by a Zobrist-style hash (units xor in, so the order units acted
  in does not matter: A-then-B and B-then-A meet in the table).
- Waiting in place is never searched: a unit that has not acted is at least as good as one
  that waited where it stood.
- Ordering: table move, winning hq capture, kills, attacks by value dealt minus counter
  taken, captures, builds, then quiet moves by history.
- Quiescence at the horizon: the side to move may finish attacks and captures; stopping is
  the stand-pat score.
- `--solve` searches to the end of the game with scores -1/0/1 (exact game value).

Weakness: with many units a few plies do not reach the end of our own turn, so the reply is
never seen. That is why the planner exists.

## Turn planner (`src/search/planner.cpp`, `plan_eval.cpp`, `threat.cpp`)

Searches whole turns.

- `PlanScore`: material + properties + funds (`Evaluate`), minus each of our units'
  expected loss to next turn's enemy fire (threat map of what every enemy can reach and hit),
  plus placement terms: capturers near properties we lack, fighters near enemies they can
  hit, indirects with enemies in their ring; a penalty for blocking our own base.
- `BeamTurns`: builds a turn one action at a time, keeping the `width` best partial turns
  (distinct positions) by PlanScore; only improving actions extend a turn. Plain moves are
  scored as a delta of the moving unit's terms without playing them.
- `PlanTurn`: negamax over turns. Our candidate turns from the beam, the reply's from its
  beam, narrowing by 2 per level; the leaf is the next side's greedy turn score. Deepens by
  two turns at a time until the clock runs out.
- Weights (`PlanWeights`) are tunable: `plan:w.exposure=80`, and `shoal tune` runs SPSA.

## Tools

- `shoal match --a ENGINE --b ENGINE --gen SIZE COUNT`: both colours on each map, Elo.
- `shoal genmap`: point-symmetric production maps for matches and tuning.
- `shoal tune`: SPSA over the planner weights by self-play.
