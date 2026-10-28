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

In planner self-play the first player won 18 of 28 decisive games on 12x12: a first-turn
edge shows up on small production maps too.
