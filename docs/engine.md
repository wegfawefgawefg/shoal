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
  hit (by terrain distance for their move type; a target on a tile the fighter cannot enter,
  like a ship for a tank or the shore for a ship, is approached from its best neighbour),
  indirects with enemies in their ring; foot soldiers riding a T-Copter or Lander valued by
  how fast the transport brings them to a property (a lander by way of a shoal or port); air
  and sea units short of fuel to get back to an own airport or port lose up to `fuel_risk`
  percent of their cost; a penalty for standing on a factory that builds something here.
- The threat map knows dived subs only fear cruisers and subs, and air units take no
  terrain cover.
- `BeamTurns`: builds a turn one action at a time, keeping the `width` best partial turns
  (distinct positions) by PlanScore; only improving actions extend a turn. Plain moves (and
  dives and surfaces) are scored as a delta of the moving unit's terms without playing them;
  generated moves carry the move points they spend, so the moved unit's fuel is known.
- `PlanTurn`: negamax over turns. Our candidate turns from the beam, the reply's from its
  beam, narrowing by 2 per level; the leaf is the next side's greedy turn score. Deepens by
  two turns at a time until the clock runs out.
- Weights (`PlanWeights`) are tunable: `plan:w.exposure=80`, and `shoal tune` runs SPSA.

## Analysis (`src/search/analysis.cpp`, `shoal analyse`)

PlanTurn's score always ends on the other side's reply (odd turn depths), so it leans against
the side to move; on the crossing planner game the two sides' turn starts are each ~8k below
zero. `AnalysePosition` therefore runs two searches: PlanTurn from the position (root), and
PlanTurn for the other side after the engine's best turn (follow, negated: one turn deeper,
our side moves last). The shown score is their mean, so turn starts of both sides line up on
one graph.

A played turn is judged at the follow depth: best = follow score, played = minus the next turn
start's root score (the same search from the position the played turn left), loss = best -
played, floored at 0. When the played turn ends where the engine's best ends, the loss is 0.
Marks: inaccuracy >= 1500, mistake >= 4000, blunder >= 8000, or not-lost becomes lost. The
plain "score here minus minus the next score" would mix the two parities and read ~-17k for
every turn of the planner's own game.

## Tools

- `shoal match --a ENGINE --b ENGINE --gen SIZE COUNT`: both colours on each map, Elo.
- `shoal genmap`: point-symmetric production maps for matches and tuning.
- `shoal tune`: SPSA over the planner weights by self-play.
