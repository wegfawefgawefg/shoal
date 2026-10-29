# Findings

What the engine has taught us so far. Player 0 moves first. No luck, no COs.

## Exact solves (tiny maps, `scripts/solve_suite.sh`)

| map | setup | perfect play |
| --- | --- | --- |
| duel3 | 3x3 plain, inf vs inf in corners, 6 days | player 0 wins |
| duel4 | 4x4 plain, inf vs inf in corners, 8 days | draw |
| duel5 | 5x5 plain, inf vs inf in corners, 8 days | draw |
| woods5 | 5x5, wood line in the middle, inf vs inf | draw |
| tanks5 | 5x5 plain, tank vs tank | draw (both sit) |
| city3 | 3x3 with a centre city, inf vs inf | player 0 wins |
| city5 | 5x5, three cities on the diagonal, inf vs inf | player 0 wins |
| hqrace4 | 4x4, hq each, inf vs inf | player 0 wins |
| twoinf5 | 5x5 plain, two inf each, 6 days | player 0 wins |
| tankinf5 | 5x5, tank (moves first) vs two inf | player 0 wins |
| hq5 | 5x5, hq + city, inf + tank each | draw with 1-4 day limits; 10 days unsolved |

The perfect-play lines are in `games/solved/` (`shoal search MAP --solve --out FILE`);
open them in the viewer to watch optimal play.

Lesson: when the armies start out of reach, whoever steps into the other's range first
gives away the first strike, so symmetric open duels are draws: neither side approaches.
Where the first mover can strike on day 1 (3x3) or there is something to take (cities,
hq), moving first wins these tiny games.

The solver's cost grows ~12-35x per extra day on hq5 (2 units a side): exact answers stay
in the tiny range; bigger questions go to the planner.

## Engine matches (generated 10x10 and 12x12 production maps, both colours)

| a | b | games | result for a | elo |
| --- | --- | --- | --- | --- |
| plan t=0.3 | greedy | 8 | +7 =0 -1 | +338 |
| plan t=0.5 turns=3 | ply t=0.5 | 24 | +18 =4 -2 | +280 |
| plan t=0.5 turns=3 | plan t=0.5 turns=1 | 24 | +14 =6 -4 | +154 |
| plan t=0.5 (deep) | plan t=0.5 turns=3 | 32 | +20 =4 -8 | +137 |
| SPSA run 1 weights | hand-set weights | 32 | +19 =9 -4 | +177 |
| SPSA run 2 weights | run 1 weights | 32 | +22 =5 -5 | +206 |
| run 2 + damage-weighted placement | run 1 weights, same code | 32 | +25 =5 -2 | +314 |
| beam 12 (t=0.3) | beam 8 | 24 | +11 =9 -4 | +104 |
| beam 16 (t=0.3) | beam 12 | 24 | +11 =5 -8 | +44 (noise) |
| beam narrows 4 per turn deeper | narrows 2 | 24 | +14 =2 -8 | +89 |
| no narrowing | narrows 2 | 24 | +9 =1 -14 | -73 |
| narrows 6 | narrows 4 | 24 | +9 =5 -10 | -14 (noise) |

In planner self-play the first player won 18 of 28 decisive games on 12x12: a first-turn
edge shows up on small production maps too.

## What self-play tuning says units are worth (SPSA, 2026-10-29)

`shoal tune` plays the planner against itself with perturbed weights and keeps what wins.
One set of weights is each unit type's worth as a percent of its cost in the static eval.
After run 2 (validated: +206 elo over the run before):

| unit | worth vs cost |
| --- | --- |
| Infantry | 181% |
| Tank | 146% |
| Anti-Air | 111% |
| Mech | 103% |
| Artillery | 94% |
| Md.Tank | 73% |
| Recon | 69% |

The same picture strong human players have: infantry are the best value in the game
(cheap, they capture, they soak hits), tanks are the efficient fighting unit, and Md.Tanks
and recons cost more than they bring. Neotank, rockets and APC were not tuned yet.

## Army composition

`shoal stats` over 12 self-play games on 12x12 maps, before and after weighting a
fighter's pull toward an enemy by the damage it would deal (instead of by its own cost):

- before: 192 anti-air built (most after day 10), trading at 0.66 funds dealt per fund lost
- after: no anti-air; infantry, tanks, neotanks and artillery; same strength in matches

## First-player edge on generated maps

Across planner self-play matches on generated 10x10 and 12x12 maps (one base each, both
colours on every map), the first player won 97 decisive games to the second player's 41.
The edge is large on small symmetric maps; real competitive maps compensate for it.
