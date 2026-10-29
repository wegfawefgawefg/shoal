# Formats

## Map

    (map
      (name "hq5")
      (day-limit 10)         ; scored at the end of player 1's turn on this day
      (funds 0 0)            ; starting funds
      (income 1000)          ; per property per turn
      (build inf mech tank)  ; optional: bases build these
      (tiles
        "h.w.."              ; one string per row, top first
        ".=...")
      (owner 0 a1)           ; owned properties; the rest are neutral
      (owner 1 e5)
      (unit 0 inf b1)        ; owner, type, tile, optional internal hp 1..100
      (unit 1 tank e4 70))

Terrain glyphs: `.` plain, `w` wood, `^` mountain, `=` road, `#` bridge, `~` river, `s` sea,
`_` shoal, `r` reef, `c` city, `b` base, `h` hq, `a` airport, `p` port.

Unit atoms: `inf mech recon tank md neo art rkt aa msl apc`.

## Game

    (game
      (map ...)
      (moves "b1 b2" "a2 c2 x c3" "end" ...)
      (evals 12 -40 ...)     ; optional engine score after each move
      (analysis              ; optional, written by `shoal analyse`
        (turn 0 1346 0 none "e6 c4, f4 d3, c3 build tank")
        (turn 9 1189 4944 mistake "o8 build mech, o5 o3 cap")))

Move text: `from to` (wait), `from to x target`, `from to cap`, `from to join`,
`from to load`, `from to drop target` (second cargo: `... drop target 2`), `tile build type`,
`end`.
Loading a game replays it and rejects illegal moves.

`(turn PLY SCORE LOSS MARK "MOVES")`, one per turn start in ply order:

- PLY: moves played before the turn starts (0, then each ply right after an `end`).
- SCORE: the engine's score there, in funds, from the view of the side to move.
- LOSS: what the turn actually played cost that side next to the engine's best (0 when it is
  the last turn or unknown). A loss that changes the result is about 1,000,000.
- MARK: `none`, `inaccuracy` (loss >= 1500), `mistake` (>= 4000), `blunder` (>= 8000, or any
  turn that turns a won or drawn position into a lost one).
- MOVES: the engine's best turn from there, move texts joined by `", "`, without the `end`;
  `""` for none. They must be legal from PLY.
