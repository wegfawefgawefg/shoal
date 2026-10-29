# shoal

An engine for Advance Wars: the aim is Stockfish-strength search for AW2-rules games, and a
native replay viewer that shows where players went wrong, like a lichess analysis board.

A shoal is a school of fish, and a beach tile in AW.

Rules: Advance Wars 2 / AWBW standard, no luck, no COs, no fog (for now). See `docs/rules.md`.

Units: infantry, mech, recon, tank, md tank, neotank, artillery, rockets, anti-air, missiles
and APC; fighter, bomber, b-copter and t-copter; battleship, cruiser, lander and sub. Fuel,
transports, joining, capture, repair at cities, airports and ports, production at bases,
airports and ports. `shoal genmap --water` makes maps with a lake, ports and airports.

![units](docs/screenshots/units.png)

## Build

```
cmake --preset release && cmake --build --preset release
./build/shoal_tests
./build/shoal show maps/tiny/hq5.sexp
./build/shoal perft maps/tiny/hq5.sexp 5
```

## Engine

Two searches (`docs/engine.md`): an exact alpha-beta solver over single unit actions for tiny
maps, and a turn planner (beam-built candidate turns, negamax over whole turns, tuned eval)
for real maps up to 32x32.

```
./build/shoal search maps/tiny/twoinf5.sexp --solve --out line.sexp   # perfect play
./build/shoal plan maps/mini8.sexp --time 2 --threads 8                # best turn
./build/shoal planplay maps/gen16-7.sexp --time 1 --out game.sexp      # self-play
./build/shoal match --a plan:t=0.5 --b plan:t=0.5:beam=8 --gen 12 16    # engine vs engine
./build/shoal tune --threads 10                                         # SPSA on the eval
./build/shoal stats out/games/                                          # builds and trades
./build/shoal book maps/mini8.sexp --turns 6 --time 20 --threads 12 --out mini8.book
./build/shoal fuzz --games 1000000 --threads 8                         # rules fuzzing
```

What it has found so far is in `docs/findings.md`; solved perfect-play lines in
`games/solved/`.

## Viewer

`shoal_viewer` (SDL3, built when CMake finds it) replays a game record or opens a map:

```
./build/shoal_viewer games/crossing-greedy.sexp
./build/shoal_viewer maps/small/crossing.sexp --screenshot out.bmp --size 1280x800 --ply 12
```

Left/right step one move, up/down one turn, home/end jump to the ends; click a move in the
list or a point on the eval graph to go there. The bar left of the board and the graph are
player 0's view: from the file's `(analysis ...)` when it has one, else its `(evals ...)`.
Turns the engine disliked carry `?!` (inaccuracy), `?` (mistake) or `??` (blunder) in the move
list and on the graph; at a turn start the engine's best turn is drawn as numbered arrows (`B`
hides them) and written in the panel.

`A` runs the planner on the shown position on a background thread (1.5 s) and shows its score
and best line; it is what makes play mode useful. `P` switches to play mode: click a unit, a
destination, then an action; the moves after the current one are replaced, `R` reloads the
file. `O` makes the engine play the other side: end your turn (`E`) and it answers with its
own turn (2 s, half the cores). `S` saves the line as it stands to `<name>-line.sexp`. `--screenshot` draws one frame with a software renderer and no window
(`--events right,up,click:X:Y,a,...` scripts input first; a live analysis is waited for).

## Analysis

```
./build/shoal analyse games/crossing-greedy.sexp --time 1 --threads 8 --out annotated.sexp
./build/shoal_viewer games/crossing-greedy-analysed.sexp
```

Every turn start is searched (two planner searches of `--time` seconds each, see
`docs/engine.md`); the table shows the score, what each played turn lost against the engine's
best, and the engine's turn next to the one played.

![analysis](docs/screenshots/analysis-marks.png)

The art is drawn from shapes in code; the font is Barlow Semi Condensed (OFL, `data/fonts`).

![replay](docs/screenshots/replay-strike.png)

## Files

Maps and games are S-expressions (`maps/`, see `docs/formats.md`). Moves are written like
`"b2 c2"` (move and wait), `"b2 c2 x c3"` (move and attack c3), `"b1 b1 cap"`,
`"a1 b1 join"`, `"a1 b1 load"`, `"b1 c1 drop c2"`, `"a1 b1 dive"`, `"a1 b1 surface"`,
`"e5 build inf"` and `"end"`. Tiles are named like chess squares, `a1` top left.

Not affiliated with Nintendo or Intelligent Systems. No game assets are included.
