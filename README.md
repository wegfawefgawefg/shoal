# shoal

An engine for Advance Wars: the aim is Stockfish-strength search for AW2-rules games, and a
native replay viewer that shows where players went wrong, like a lichess analysis board.

A shoal is a school of fish, and a beach tile in AW.

Rules: Advance Wars 2 / AWBW standard, no luck, no COs, no fog (for now). See `docs/rules.md`.

## Build

```
cmake --preset release && cmake --build --preset release
./build/shoal_tests
./build/shoal show maps/tiny/hq5.sexp
./build/shoal perft maps/tiny/hq5.sexp 5
```

## Viewer

`shoal_viewer` (SDL3, built when CMake finds it) replays a game record or opens a map:

```
./build/shoal_viewer games/crossing-greedy.sexp
./build/shoal_viewer maps/small/crossing.sexp --screenshot out.bmp --size 1280x800 --ply 12
```

Left/right step one move, up/down one turn, home/end jump to the ends; click a move in the
list to go there. The bar left of the board is the eval from `(evals ...)`, from player 0's
view. `P` switches to play mode: click a unit, a destination, then an action; the moves after
the current one are replaced, `R` reloads the file. `--screenshot` draws one frame with a
software renderer and no window (`--events right,up,click:X:Y,...` scripts input first).

The art is drawn from shapes in code; the font is Barlow Semi Condensed (OFL, `data/fonts`).

![replay](docs/screenshots/replay-strike.png)

## Files

Maps and games are S-expressions (`maps/`, see `docs/formats.md`). Moves are written like
`"b2 c2"` (move and wait), `"b2 c2 x c3"` (move and attack c3), `"b1 b1 cap"`,
`"e5 build inf"` and `"end"`. Tiles are named like chess squares, `a1` top left.

Not affiliated with Nintendo or Intelligent Systems. No game assets are included.
